#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

#include "mock_services.h"
#include "mock_llmservices.h"

#include "../../Source/Application/LLMServices.h"
#include "../../Source/Application/ChatImpl.h"


class LLMServicesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_construction_and_defaults();
    void test_api_management();
    void test_model_aggregation();
    void test_is_service_available();
    void test_receive_parsing();
    void test_config_persistence();
    void test_stop_function();
    void test_post_function();
    void test_get_available_models();
    void test_get_embedding();
    void test_error_handling();
    void test_edge_cases();
};

void LLMServicesTest::initTestCase()
{
    qDebug() << "LLMServicesTest::initTestCase()";

    ApplicationServices mockservice(this);
    mockservice.initialize();   

    // Enregistrer le MockService pour tous les types
    LLMService::registerService<MockLLMService>(static_cast<int>(LLMEnum::LLMType::LlamaCpp));
    LLMService::registerService<MockLLMService>(static_cast<int>(LLMEnum::LLMType::Ollama));
    LLMService::registerService<MockLLMService>(static_cast<int>(LLMEnum::LLMType::OpenAI));

    // Supprimer le fichier de config au cas où pour démarrer propre
    if (QFile::exists("LLMService.json"))
        QFile::remove("LLMService.json");
}

void LLMServicesTest::cleanupTestCase()
{
    if (QFile::exists("LLMService.json"))
        QFile::remove("LLMService.json");

    qDebug() << "LLMServicesTest::cleanupTestCase()";
}

void LLMServicesTest::test_construction_and_defaults()
{
    qDebug() << "LLMServicesTest::test_construction_and_defaults()";
    LLMServices services(nullptr);
    // Comme on a enregistré les factory, createDefaultServiceJsonFile devrait avoir ajouté les APIs par défaut
    QVERIFY(services.getAPIs().size() >= 1); 
    QVERIFY(services.get("LlamaCpp") != nullptr);
}

void LLMServicesTest::test_api_management()
{
    qDebug() << "LLMServicesTest::test_api_management()";
    LLMServices services(nullptr);
    
    // 1. Test de récupération avec nom unique CustomAPI (pour éviter le conflit avec Ollama/LlamaCpp par défaut)
    MockLLMService* m1 = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "CustomAPI");
    services.addAPI(m1);
    QCOMPARE(services.get("CustomAPI"), (LLMService*)m1);

    // 2. Test de récupération par type (en utilisant OpenAI qui n'est pas ajouté par défaut)
    QString openAIName = enumValueToString<LLMEnum>("LLMType", LLMEnum::LLMType::OpenAI);
    MockLLMService* m2 = new MockLLMService(LLMEnum::LLMType::OpenAI, &services, openAIName);
    services.addAPI(m2);

    QCOMPARE(services.get(openAIName), (LLMService*)m2);
    QCOMPARE(services.get(LLMEnum::LLMType::OpenAI), (LLMService*)m2);
}

void LLMServicesTest::test_model_aggregation()
{
    qDebug() << "LLMServicesTest::test_model_aggregation()";
    LLMServices services(nullptr);
    MockLLMService* m1 = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "M1");
    m1->addModel("model-a");
    services.addAPI(m1);

    LLMModel model = services.getModel(model.toString());
    QCOMPARE(model.name_, QString("model-a"));
    QCOMPARE(model.num_params_, QString("7B"));
    QCOMPARE(model.toString(), QString("model-a:7B"));
}

void LLMServicesTest::test_is_service_available()
{
    qDebug() << "LLMServicesTest::test_is_service_available()";
    LLMServices services(nullptr);
    QString openAIName = enumValueToString<LLMEnum>("LLMType", LLMEnum::LLMType::OpenAI);
    MockLLMService* m1 = new MockLLMService(LLMEnum::LLMType::OpenAI, &services, openAIName);
    services.addAPI(m1);

    QVERIFY(!services.isServiceAvailable(LLMEnum::LLMType::OpenAI));
    m1->setReady(true);
    QVERIFY(services.isServiceAvailable(LLMEnum::LLMType::OpenAI));
    m1->setReady(false);
    QVERIFY(!services.isServiceAvailable(LLMEnum::LLMType::OpenAI));    
}

void LLMServicesTest::test_receive_parsing()
{
    qDebug() << "LLMServicesTest::test_receive_parsing()";
    LLMServices services(nullptr);
    MockLLMService* api = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "ParserAPI");
    ChatImpl chat(&services);
    
    chat.updateContent("Trigger"); 
    
    QByteArray data = "{\"response\": \"Hello\"}\n{\"response\": \" World\"}";
    services.receive(api, &chat, data);
    
    QVERIFY(chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString().contains("Hello World"));
}

void LLMServicesTest::test_config_persistence()
{
    qDebug() << "LLMServicesTest::test_config_persistence()";
    // Sauvegarder
    {
        LLMServices services(nullptr);
        MockLLMService* m = new MockLLMService(LLMEnum::LLMType::OpenAI, &services, "PersistAPI");
        services.addAPI(m);
        services.saveServiceJsonFile();
    }

    // Charger
    {
        LLMServices services2(nullptr);
        LLMService* loaded = services2.get("PersistAPI");
        QVERIFY(loaded != nullptr);
        QCOMPARE(loaded->name_, QString("PersistAPI"));
    }
}

void LLMServicesTest::test_stop_function()
{
    qDebug() << "LLMServicesTest::test_stop_function()";
    LLMServices services(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "StopAPI");
    services.addAPI(mock);
    
    ChatImpl chat(&services);
    chat.setApi("StopAPI");
    
    // Vérifier que stop ne crash pas
    services.stop(&chat);
    QVERIFY(true); // Si on arrive ici, c'est que ça n'a pas crashé
}

void LLMServicesTest::test_post_function()
{
    qDebug() << "LLMServicesTest::test_post_function()";
    LLMServices services(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "PostAPI") ;
    services.addAPI(mock);
    
    ChatImpl chat(&services);
    chat.setApi("PostAPI");
    
    // Vérifier que post ne crash pas
    services.post(mock, &chat, "Test message");
    QVERIFY(true); // Si on arrive ici, c'est que ça n'a pas crashé
}

void LLMServicesTest::test_get_available_models()
{
    qDebug() << "LLMServicesTest::test_get_available_models()";
    LLMServices services(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "ModelsAPI");
    mock->addModel("model-1");
    mock->addModel("model-2");
    services.addAPI(mock);
    
    // Test avec API spécifié
    std::vector<LLMModel> models = services.getAvailableModels(mock);
    QCOMPARE(static_cast<int>(models.size()), 2);
    
    // Test avec nullptr (devrait retourner vecteur vide)
    std::vector<LLMModel> emptyModels = services.getAvailableModels(nullptr);
    QVERIFY(emptyModels.empty());
}

void LLMServicesTest::test_get_embedding()
{
    qDebug() << "LLMServicesTest::test_get_embedding()";
    LLMServices services(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "EmbeddingAPI");
    services.addAPI(mock);
    
    // Test avec un service qui ne supporte pas les embeddings
    std::vector<float> embedding = services.getEmbedding("test text");
    QVERIFY(embedding.empty());
}

void LLMServicesTest::test_error_handling()
{
    qDebug() << "LLMServicesTest::test_error_handling()";
    LLMServices services(nullptr);
    MockLLMService* mock = new MockLLMService(LLMEnum::LLMType::LlamaCpp, &services, "ErrorAPI");
    services.addAPI(mock);
    
    ChatImpl chat(&services);
    
    // Test receive avec données invalides
    QByteArray invalidData = "invalid json data";
    services.receive(mock, &chat, invalidData);
    QVERIFY(true); // Ne devrait pas crasher
    
    // Test receive avec JSON contenant une erreur
    QByteArray errorData = "{\"error\": \"Test error\"}";
    services.receive(mock, &chat, errorData);
    QVERIFY(true); // Ne devrait pas crasher
}

void LLMServicesTest::test_edge_cases()
{
    qDebug() << "LLMServicesTest::test_edge_cases()";
    LLMServices services(nullptr);
    
    // Test get avec nom inexistant
    LLMService* nullService = services.get("NonExistentAPI");
    QVERIFY(nullService == nullptr);
    
    // Test get avec type inexistant
    LLMService* nullTypeService = services.get(LLMEnum::LLMType::OpenAI);
    QVERIFY(nullTypeService == nullptr);
    
    // Test isServiceAvailable avec service inexistant
    bool available = services.isServiceAvailable(LLMEnum::LLMType::OpenAI);
    QVERIFY(!available);
    
    // Test getModel avec modèle inexistant
    LLMModel emptyModel = services.getModel("NonExistentModel");
    QVERIFY(emptyModel.name_.isEmpty());
}

QTEST_MAIN(LLMServicesTest)
#include "tst_llmservices.moc"
