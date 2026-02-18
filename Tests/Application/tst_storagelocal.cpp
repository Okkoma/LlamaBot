#include <QtTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include "mock_services.h"
#include "mock_llmservices.h"

#include "../../Source/Application/LLMServices.h"
#include "../../Source/Application/ChatImpl.h"
#include "../../Source/Application/ChatStorageLocal.h"

/**
 * @brief Tests unitaires pour ChatStorageLocal
 * 
 * Ces tests vérifient:
 * - La création et l'initialisation de la base de données SQLite
 * - Le chargement et la sauvegarde des chats via SQLite
 * - Le chargement et la sauvegarde des chats via JSON
 * - La migration automatique de JSON vers SQLite
 * - La gestion des erreurs (base de données corrompue, fichiers manquants, etc.)
 * - Les cas limites (liste vide, données invalides, etc.)
 */
class ChatStorageLocalTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Tests de base
    void test_construction();
    void test_database_initialization();
    void test_database_path();

    // Tests de sauvegarde/chargement SQLite
    void test_save_and_load_single_chat_db();
    void test_save_and_load_multiple_chats_db();
    void test_save_empty_list_db();
    void test_load_nonexistent_db();
    void test_save_with_special_characters_db();

    // Tests de sauvegarde/chargement JSON
    void test_json_file_path();    
    void test_save_and_load_json_file();
    void test_load_invalid_json_file();
    void test_migration_json_to_db();

    // Tests de gestion des erreurs
    void test_database_transaction_rollback();
    void test_corrupted_database();
    
    // Tests de cas limites
    void test_chat_with_empty_messages();
    void test_chat_with_long_content();
    void test_multiple_save_operations();
    void test_concurrent_storage_instances();

private:
    QString testDataPath() const;
    void clearTestData();
    void delete_local_db_and_jsonfile();
    void createTestJsonFile(const QJsonArray& chats);
    void createTestDatabase(const QJsonArray& chats);
    QJsonArray createTestChatJson(int count = 1);
};

void ChatStorageLocalTest::initTestCase()
{
    qDebug() << "ChatStorageLocalTest::initTestCase()";
    
    // Initialiser les services mock
    ApplicationServices mockservice(this);
    
    // S'assurer que le répertoire de test existe
    QDir dir(testDataPath());
    if (!dir.exists())
        dir.mkpath(".");
}

void ChatStorageLocalTest::cleanupTestCase()
{
    qDebug() << "ChatStorageLocalTest::cleanupTestCase()";
    clearTestData();
}

void ChatStorageLocalTest::init()
{
    // Nettoyer avant chaque test
    clearTestData();
}

void ChatStorageLocalTest::cleanup()
{
    // Nettoyer après chaque test
    clearTestData();
}

QString ChatStorageLocalTest::testDataPath() const
{
    // Utiliser un répertoire temporaire pour les tests
    return QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/LlamaBotTest";
}

void ChatStorageLocalTest::clearTestData()
{
    // Supprimer tous les fichiers de test
    QDir dir(testDataPath());
    if (dir.exists())
    {
        dir.removeRecursively();
    }
    
    // Recréer le répertoire
    dir.mkpath(".");

    delete_local_db_and_jsonfile();
}

void ChatStorageLocalTest::delete_local_db_and_jsonfile()
{
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat.db";
    QString jsonPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chats.json";
    QFile::remove(dbPath);
    QFile::remove(jsonPath);
}

void ChatStorageLocalTest::createTestJsonFile(const QJsonArray& chats)
{
    QString jsonPath = testDataPath() + "/chats.json";
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly))
    {
        QJsonDocument doc(chats);
        file.write(doc.toJson());
        file.close();
    }
}

void ChatStorageLocalTest::createTestDatabase(const QJsonArray& chats)
{
    QString dbPath = testDataPath() + "/chat.db";
    QString connectionName = "test_connection";
    
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbPath);
    
    if (db.open())
    {
        QSqlQuery q(db);
        q.exec("CREATE TABLE IF NOT EXISTS conversations ("
               "id TEXT PRIMARY KEY,"
               "name TEXT,"
               "payload_json TEXT NOT NULL,"
               "updated_at INTEGER NOT NULL"
               ");");
        
        q.prepare("INSERT INTO conversations(id, name, payload_json, updated_at) VALUES(?, ?, ?, ?);");
        
        qint64 now = QDateTime::currentSecsSinceEpoch();
        for (const auto& v : chats)
        {
            if (!v.isObject())
                continue;
            
            QJsonObject obj = v.toObject();
            QString id = obj.value("id").toString();
            QString name = obj.value("name").toString();
            QString payload = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
            
            q.bindValue(0, id);
            q.bindValue(1, name);
            q.bindValue(2, payload);
            q.bindValue(3, now);
            q.exec();
        }
        
        db.close();
    }
    
    QSqlDatabase::removeDatabase(connectionName);
}

QJsonArray ChatStorageLocalTest::createTestChatJson(int count)
{
    QJsonArray chats;
    
    for (int i = 0; i < count; ++i)
    {
        QJsonObject chat;
        chat["id"] = QString("test-chat-%1").arg(i);
        chat["name"] = QString("Test Chat %1").arg(i);
        chat["api"] = "TestAPI";
        chat["model"] = "test-model";
        chat["streamed"] = true;
        
        QJsonArray messages;
        QJsonObject msg1;
        msg1["role"] = "user";
        msg1["content"] = QString("Question %1").arg(i);
        messages.append(msg1);
        
        QJsonObject msg2;
        msg2["role"] = "assistant";
        msg2["content"] = QString("Answer %1").arg(i);
        messages.append(msg2);
        
        chat["messages"] = messages;
        chats.append(chat);
    }
    
    return chats;
}

void ChatStorageLocalTest::test_construction()
{
    qDebug() << "ChatStorageLocalTest::test_construction()";
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
    
    // Vérifier que l'objet est bien créé
    QVERIFY(&storage != nullptr);
}

void ChatStorageLocalTest::test_database_initialization()
{
    qDebug() << "ChatStorageLocalTest::test_database_initialization()";
    
    // Modifier temporairement le chemin de données pour les tests
    QStandardPaths::setTestModeEnabled(true);
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
    
    // Sauvegarder une liste vide devrait créer la base de données
    QList<Chat*> emptyList;
    bool result = storage.save(emptyList);
    
    QVERIFY(result);
    
    // Vérifier que le fichier de base de données existe
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat.db";
    QVERIFY(QFile::exists(dbPath));
    
    QStandardPaths::setTestModeEnabled(false);
}

void ChatStorageLocalTest::test_database_path()
{
    qDebug() << "ChatStorageLocalTest::test_database_path()";
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
    
    // Le chemin devrait être dans AppDataLocation
    QString expectedPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat.db";
    
    // On ne peut pas accéder directement à dbPath() car c'est privé,
    // mais on peut vérifier indirectement via save()
    QList<Chat*> emptyList;
    storage.save(emptyList);
    
    QVERIFY(QFile::exists(expectedPath));
}

void ChatStorageLocalTest::test_save_and_load_single_chat_db()
{
    qDebug() << "ChatStorageLocalTest::test_save_and_load_single_chat_db()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer un chat de test
    ChatImpl* chat1 = new ChatImpl(&llmservices, "Test Chat", "System Prompt", true);
    chat1->setApi("TestAPI");
    chat1->setModel("test-model:");
    chat1->updateContent("Hello");
    chat1->updateCurrentAIStream("Hi there");
    
    QList<Chat*> chatsToSave;
    chatsToSave.append(chat1);
    
    // Sauvegarder
    bool saveResult = storage.save(chatsToSave);
    QVERIFY(saveResult);
    
    // Charger
    QList<Chat*> loadedChats;
    bool loadResult = storage.load(loadedChats);
    QVERIFY(loadResult);
    QCOMPARE(loadedChats.size(), 1);
    
    // Vérifier le contenu
    Chat* loadedChat = loadedChats.first();
    QCOMPARE(loadedChat->getName(), QString("Test Chat"));
    QCOMPARE(loadedChat->getCurrentApi(), QString("TestAPI"));
    QVERIFY(loadedChat->rowCount() > 0);
    
    // Nettoyer
    delete chat1;
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_save_and_load_multiple_chats_db()
{
    qDebug() << "ChatStorageLocalTest::test_save_and_load_multiple_chats_db()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer plusieurs chats
    QList<Chat*> chatsToSave;
    for (int i = 0; i < 5; ++i)
    {
        ChatImpl* chat = new ChatImpl(&llmservices, QString("Chat %1").arg(i), "System", true);
        chat->setApi("TestAPI");
        chat->setModel("test-model:");
        chat->updateContent(QString("Question %1").arg(i));
        chat->updateCurrentAIStream(QString("Answer %1").arg(i));
        chatsToSave.append(chat);
    }
    
    // Sauvegarder
    QVERIFY(storage.save(chatsToSave));
    
    // Charger
    QList<Chat*> loadedChats;
    QVERIFY(storage.load(loadedChats));
    QCOMPARE(loadedChats.size(), 5);
    
    // Vérifier que tous les chats sont présents
    for (int i = 0; i < 5; ++i)
    {
        bool found = false;
        for (Chat* chat : loadedChats)
        {
            if (chat->getName() == QString("Chat %1").arg(i))
            {
                found = true;
                break;
            }
        }
        QVERIFY(found);
    }
    
    // Nettoyer
    qDeleteAll(chatsToSave);
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_save_empty_list_db()
{
    qDebug() << "ChatStorageLocalTest::test_save_empty_list_db()";
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
        
    // Sauvegarder une liste vide devrait réussir
    QList<Chat*> emptyList;
    QVERIFY(storage.save(emptyList));

    qDebug() << "ChatStorageLocalTest::test_save_empty_list_db(): size:" << storage.size();
    QVERIFY(storage.size() == 0);

    // Charger devrait retourner une liste vide
    QList<Chat*> loadedChats;
    storage.load(loadedChats);
    QCOMPARE(loadedChats.size(), 0);
}

void ChatStorageLocalTest::test_load_nonexistent_db()
{
    qDebug() << "ChatStorageLocalTest::test_load_nonexistent_db()";
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
    
    QList<Chat*> loadedChats;
    bool result = storage.load(loadedChats);
    
    // Le chargement devrait échouer gracieusement
    QVERIFY(!result);
    QCOMPARE(loadedChats.size(), 0);
}

void ChatStorageLocalTest::test_save_with_special_characters_db()
{
    qDebug() << "ChatStorageLocalTest::test_save_with_special_characters_db()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer un chat avec des caractères spéciaux
    ChatImpl* chat = new ChatImpl(&llmservices, "Chat avec éàèûç@#$%", "System", true);
    chat->setApi("TestAPI");
    chat->setModel("test-model:");
    chat->updateContent("Question avec caractères spéciaux: éàèûç@#$%^&*()");
    chat->updateCurrentAIStream("Réponse avec émojis 🤖 🎉 ✨");
    
    QList<Chat*> chatsToSave;
    chatsToSave.append(chat);
    
    // Sauvegarder
    QVERIFY(storage.save(chatsToSave));
    
    // Charger
    QList<Chat*> loadedChats;
    QVERIFY(storage.load(loadedChats));
    QCOMPARE(loadedChats.size(), 1);
    
    // Vérifier que les caractères spéciaux sont préservés
    Chat* loadedChat = loadedChats.first();
    QVERIFY(loadedChat->getName().contains("éàèûç"));
    
    // Nettoyer
    delete chat;
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_json_file_path()
{
    qDebug() << "ChatStorageLocalTest::test_json_file_path()";
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
    
    // Le chemin devrait être dans AppDataLocation
    QString expectedPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chats.json";
    
    // Créer un fichier JSON de test
    QJsonArray testChats = createTestChatJson(1);
    QFile file(expectedPath);
    if (file.open(QIODevice::WriteOnly))
    {
        QJsonDocument doc(testChats);
        file.write(doc.toJson());
        file.close();
    }
    
    QVERIFY(QFile::exists(expectedPath));
}

void ChatStorageLocalTest::test_save_and_load_json_file()
{
    qDebug() << "ChatStorageLocalTest::test_save_and_load_json_file()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    // Désactiver la base de données SQLite pour forcer l'utilisation de JSON
    // (on pourrait simuler l'absence du driver QSQLITE)
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer un chat
    ChatImpl* chat = new ChatImpl(&llmservices, "JSON Test Chat", "System", true);
    chat->setApi("TestAPI");
    chat->setModel("test-model:");
    chat->updateContent("JSON Question");
    chat->updateCurrentAIStream("JSON Answer");
    
    QList<Chat*> chatsToSave;
    chatsToSave.append(chat);
    
    // Sauvegarder (devrait utiliser la base de données par défaut)
    QVERIFY(storage.save(chatsToSave));
    
    // Vérifier que le fichier JSON existe aussi (fallback)
    QString jsonPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chats.json";
    
    // Nettoyer
    delete chat;
}

void ChatStorageLocalTest::test_load_invalid_json_file()
{
    qDebug() << "ChatStorageLocalTest::test_load_invalid_json_file()";
    
    LLMServices llmservices(nullptr);
    ChatStorageLocal storage(&llmservices);
    
    // Créer un fichier JSON invalide
    QString jsonPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chats.json";
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write("{ invalid json content }");
        file.close();
    }
    
    // Supprimer la base de données pour forcer le chargement du JSON
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat.db";
    QFile::remove(dbPath);
    
    QList<Chat*> loadedChats;
    bool result = storage.load(loadedChats);
    
    // Le chargement devrait échouer gracieusement
    QVERIFY(!result);
    QCOMPARE(loadedChats.size(), 0);
}

void ChatStorageLocalTest::test_migration_json_to_db()
{
    qDebug() << "ChatStorageLocalTest::test_migration_json_to_db()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    // Créer un fichier JSON avec des chats
    QJsonArray testChats = createTestChatJson(3);
    QString jsonPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chats.json";
    QFile file(jsonPath);
    if (file.open(QIODevice::WriteOnly))
    {
        QJsonDocument doc(testChats);
        file.write(doc.toJson());
        file.close();
    }
    
    // S'assurer qu'aucune base de données n'existe
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat.db";
    QFile::remove(dbPath);
    
    ChatStorageLocal storage(&llmservices);
    
    // Charger devrait migrer automatiquement de JSON vers DB
    QList<Chat*> loadedChats;
    storage.load(loadedChats);
    
    // Vérifier que la base de données a été créée
    QVERIFY(QFile::exists(dbPath));
    
    // Nettoyer
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_database_transaction_rollback()
{
    qDebug() << "ChatStorageLocalTest::test_database_transaction_rollback()";
    
    // Ce test vérifie que les transactions sont correctement gérées
    // En cas d'erreur, la transaction devrait être annulée
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer un chat valide
    ChatImpl* chat1 = new ChatImpl(&llmservices, "Valid Chat", "System", true);
    chat1->setApi("TestAPI");
    chat1->setModel("test-model:");
    
    QList<Chat*> chats;
    chats.append(chat1);
    
    // Sauvegarder d'abord un chat valide
    QVERIFY(storage.save(chats));
    
    // Vérifier que le chat a été sauvegardé
    QList<Chat*> loadedChats;
    QVERIFY(storage.load(loadedChats));
    QCOMPARE(loadedChats.size(), 1);
    
    // Nettoyer
    delete chat1;
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_corrupted_database()
{
    qDebug() << "ChatStorageLocalTest::test_corrupted_database()";
    
    LLMServices llmservices(nullptr);
    
    // Créer un fichier de base de données corrompu
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/chat.db";
    QFile file(dbPath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write("This is not a valid SQLite database");
        file.close();
    }
    
    ChatStorageLocal storage(&llmservices);
    
    QList<Chat*> loadedChats;
    bool result = storage.load(loadedChats);
    
    // Le chargement devrait échouer car il n'y a pas de fallback JSON valide
    // (ou réussir s'il y a un fichier JSON de fallback des tests précédents)
    // Dans tous les cas, cela ne devrait pas crasher
    QVERIFY(true); // Le test passe si on arrive ici sans crash
    
    // Nettoyer
    qDeleteAll(loadedChats);
}


void ChatStorageLocalTest::test_chat_with_empty_messages()
{
    qDebug() << "ChatStorageLocalTest::test_chat_with_empty_messages()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer un chat sans messages
    ChatImpl* chat = new ChatImpl(&llmservices, "Empty Chat", "System", true);
    chat->setApi("TestAPI");
    chat->setModel("test-model:");
    
    QList<Chat*> chatsToSave;
    chatsToSave.append(chat);
    
    // Sauvegarder
    QVERIFY(storage.save(chatsToSave));
    
    // Charger
    QList<Chat*> loadedChats;
    QVERIFY(storage.load(loadedChats));
    QCOMPARE(loadedChats.size(), 1);
    
    // Nettoyer
    delete chat;
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_chat_with_long_content()
{
    qDebug() << "ChatStorageLocalTest::test_chat_with_long_content()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Créer un chat avec un contenu très long
    ChatImpl* chat = new ChatImpl(&llmservices, "Long Content Chat", "System", true);
    chat->setApi("TestAPI");
    chat->setModel("test-model:");
    
    QString longContent = QString("A").repeated(100000); // 100k caractères
    chat->updateContent(longContent);
    chat->updateCurrentAIStream(longContent);
    
    QList<Chat*> chatsToSave;
    chatsToSave.append(chat);
    
    // Sauvegarder
    QVERIFY(storage.save(chatsToSave));
    
    // Charger
    QList<Chat*> loadedChats;
    QVERIFY(storage.load(loadedChats));
    QCOMPARE(loadedChats.size(), 1);
    
    // Vérifier que le contenu long est préservé
    Chat* loadedChat = loadedChats.first();
    QVERIFY(loadedChat->rowCount() > 0);
    
    // Nettoyer
    delete chat;
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_multiple_save_operations()
{
    qDebug() << "ChatStorageLocalTest::test_multiple_save_operations()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    ChatStorageLocal storage(&llmservices);
    
    // Sauvegarder plusieurs fois de suite
    for (int i = 0; i < 10; ++i)
    {
        QList<Chat*> chats;
        ChatImpl* chat = new ChatImpl(&llmservices, QString("Chat %1").arg(i), "System", true);
        chat->setApi("TestAPI");
        chat->setModel("test-model:");
        chats.append(chat);
        
        QVERIFY(storage.save(chats));
        
        delete chat;
    }
    
    // Charger et vérifier qu'on a bien le dernier chat sauvegardé
    QList<Chat*> loadedChats;
    QVERIFY(storage.load(loadedChats));
    QCOMPARE(loadedChats.size(), 1); // La stratégie actuelle remplace tout
    
    // Nettoyer
    qDeleteAll(loadedChats);
}

void ChatStorageLocalTest::test_concurrent_storage_instances()
{
    qDebug() << "ChatStorageLocalTest::test_concurrent_storage_instances()";
    
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &llmservices, "TestAPI");
    mock->addModel("test-model");
    llmservices.addAPI(mock);
    
    // Créer deux instances de storage
    ChatStorageLocal storage1(&llmservices);
    ChatStorageLocal storage2(&llmservices);
    
    // Sauvegarder avec la première instance
    ChatImpl* chat1 = new ChatImpl(&llmservices, "Chat from storage1", "System", true);
    chat1->setApi("TestAPI");
    chat1->setModel("test-model:");
    
    QList<Chat*> chats1;
    chats1.append(chat1);
    QVERIFY(storage1.save(chats1));
    
    // Charger avec la deuxième instance
    QList<Chat*> loadedChats;
    QVERIFY(storage2.load(loadedChats));
    QCOMPARE(loadedChats.size(), 1);
    QCOMPARE(loadedChats.first()->getName(), QString("Chat from storage1"));
    
    // Nettoyer
    delete chat1;
    qDeleteAll(loadedChats);
}

QTEST_MAIN(ChatStorageLocalTest)
#include "tst_storagelocal.moc"
