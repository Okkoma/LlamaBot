#include <QtTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>

#include "../Source/Application/LLMServices.h"
#include "../Source/Application/ChatImpl.h"

// Mock amélioré pour tester les interactions avec les services LLM
class MockLLMService : public LLMService {
public:
    MockLLMService(LLMServices* s, const QString& name, LLMEnum::LLMType type) 
        : LLMService(static_cast<int>(type), s, name) {
        type_ = static_cast<int>(type);
    }

    std::vector<LLMModel> getAvailableModels() const override {
        return models_;
    }

    void addModel(const QString& name, const QString& filePath = "") {
        LLMModel model;
        model.name_ = name;
        model.filePath_ = filePath;
        models_.push_back(model);
    }

    bool isReady() const override { return true; }

    std::vector<LLMModel> models_;
};

class ChatTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_construction();
    void test_initialization_with_apis();
    void test_set_api_and_model_signals();
    void test_auto_switch_ollama_to_llamacpp();
    void test_update_content_and_history();
    void test_streaming_and_sanitization();
    void test_streaming_and_end_tag();
    void test_serialization_json();
    void test_export_helpers();
    void test_finalize_stream();
    void test_error_handling();
    void test_edge_cases();
};

void ChatTest::initTestCase()
{
    qDebug() << "LLMServicesTest::initTestCase()";
}

void ChatTest::test_construction()
{
    qDebug() << "LLMServicesTest::test_construction()";
    LLMServices llmservices(nullptr);
    ChatImpl chat(&llmservices, "TestChat", "System Prompt", true);
    
    QCOMPARE(chat.getName(), QString("TestChat"));
    QCOMPARE(chat.getStreamed(), true);
    QVERIFY(chat.getMessages().isEmpty());
    QCOMPARE(chat.historyList().size(), 0);
}

void ChatTest::test_initialization_with_apis()
{
    qDebug() << "LLMServicesTest::test_initialization_with_apis()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "MockAPI", LLMEnum::LLMType::LlamaCpp);
    mock->addModel("model1");
    llmservices.addAPI(mock);

    // L'initialisation se fait dans le constructeur de ChatImpl
    ChatImpl chat(&llmservices);
    
    // Devrait avoir sélectionné la première API et son premier modèle
    QCOMPARE(chat.getCurrentApi(), QString("MockAPI"));
    QCOMPARE(chat.getCurrentModel(), QString("model1:")); // toString() format
}

void ChatTest::test_set_api_and_model_signals()
{
    qDebug() << "LLMServicesTest::test_set_api_and_model_signals()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "API1", LLMEnum::LLMType::LlamaCpp);
    mock->addModel("m1");
    mock->addModel("m2");
    llmservices.addAPI(mock);

    ChatImpl chat(&llmservices);
    // Au démarrage, "m1:" est déjà sélectionné.
    QSignalSpy modelSpy(&chat, &Chat::currentModelChanged);

    chat.setModel("m2:");
    QCOMPARE(modelSpy.count(), 1);
    QCOMPARE(chat.getCurrentModel(), QString("m2:"));
}

void ChatTest::test_auto_switch_ollama_to_llamacpp()
{
    qDebug() << "LLMServicesTest::test_auto_switch_ollama_to_llamacpp()";
    LLMServices llmservices(nullptr);
    
    MockLLMService* ollama = new MockLLMService(&llmservices, "Ollama", LLMEnum::LLMType::Ollama);
    // Ollama ne connaît pas ce modèle localement
    llmservices.addAPI(ollama);

    MockLLMService* llamacpp = new MockLLMService(&llmservices, "LlamaCpp", LLMEnum::LLMType::LlamaCpp);
    llamacpp->addModel("local-model", "test.gguf");
    llmservices.addAPI(llamacpp);

    ChatImpl chat(&llmservices);
    chat.setApi("Ollama");
    
    // On sélectionne le modèle local. getModel le trouvera dans LlamaCpp avec son chemin .gguf
    chat.setModel("local-model:");
    
    QCOMPARE(chat.getCurrentApi(), QString("LlamaCpp"));
    QCOMPARE(chat.getCurrentModel(), QString("local-model:"));
}

void ChatTest::test_update_content_and_history()
{
    qDebug() << "LLMServicesTest::test_update_content_and_history()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "Mock", LLMEnum::LLMType::LlamaCpp);
    llmservices.addAPI(mock);

    ChatImpl chat(&llmservices);
    QSignalSpy historySpy(&chat, &Chat::historyChanged);
    QSignalSpy messageAddedSpy(&chat, &Chat::messageAdded);

    chat.updateContent("User Question");

    // updateContent ajoute un message 'user' ET un bloc 'assistant' vide pour le futur stream
    QCOMPARE(chat.getMessages().size(), 2);
    // messageAdded doit avoir été émis pour le user et pour le bloc assistant vide
    QCOMPARE(messageAddedSpy.count(), 2);
    
    // L'historique doit contenir le message utilisateur 
    // et le bloc assistant vide qui n'a pas encore reçu de stream/finalize
    QCOMPARE(chat.historyList().size(), 2);
    QCOMPARE(chat.historyList().at(0).toMap()["role"].toString(), QString("user"));
    QCOMPARE(chat.historyList().at(0).toMap()["content"].toString(), QString("User Question"));
    QCOMPARE(chat.historyList().last().toMap()["role"].toString(), QString("assistant"));
    QCOMPARE(chat.historyList().last().toMap()["content"].toString(), QString(""));    
}

void ChatTest::test_streaming_and_sanitization()
{
    qDebug() << "LLMServicesTest::test_streaming_and_sanitization()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "Mock", LLMEnum::LLMType::LlamaCpp);
    llmservices.addAPI(mock);

    ChatImpl chat(&llmservices);
    chat.updateContent("Ask"); 
    
    // Simuler le stream avec caractères à nettoyer
    chat.updateCurrentAIStream("|Hello");
    
    // On vérifie l'historique qui contient le contenu brut (sans le prompt "🤖 >")
    QCOMPARE(chat.historyList().last().toMap()["content"].toString(), QString("Hello"));

    // Simuler un autre début de stream avec '>'
    chat.updateContent("Ask2");
    chat.updateCurrentAIStream(">Quoted content");
    QCOMPARE(chat.historyList().last().toMap()["content"].toString(), QString("Quoted content"));
}

void ChatTest::test_streaming_and_end_tag()
{
    qDebug() << "LLMServicesTest::test_streaming_and_end_tag()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "Mock", LLMEnum::LLMType::LlamaCpp);
    llmservices.addAPI(mock);

    ChatImpl chat(&llmservices);
    chat.updateContent("Ask"); 
    // On vérifie l'historique qui contient le contenu de la question
    QCOMPARE(chat.historyList()[chat.historyList().size()-2].toMap()["content"].toString(), QString("Ask"));

    // Simuler le stream avec une pensée + réponse + tag end pour finaliser le stream
    chat.updateCurrentAIStream("<think>Thought1</think>Response1<think>Thought2</think><think>Thought3</think>Response2<end>");
    
    // On vérifie l'historique qui contient les differents messages de la réponse (sans le prompt ai)
    QCOMPARE(chat.historyList()[chat.historyList().size()-5].toMap()["content"].toString(), QString("Thought1"));
    QCOMPARE(chat.historyList()[chat.historyList().size()-4].toMap()["content"].toString(), QString("Response1"));
    QCOMPARE(chat.historyList()[chat.historyList().size()-3].toMap()["content"].toString(), QString("Thought2"));
    QCOMPARE(chat.historyList()[chat.historyList().size()-2].toMap()["content"].toString(), QString("Thought3"));
    QCOMPARE(chat.historyList()[chat.historyList().size()-1].toMap()["content"].toString(), QString("Response2"));
}

void ChatTest::test_serialization_json()
{
    qDebug() << "LLMServicesTest::test_serialization_json()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "API", LLMEnum::LLMType::LlamaCpp);
    llmservices.addAPI(mock);

    ChatImpl chat1(&llmservices, "Original", "System", false);
    chat1.setApi("API");
    chat1.updateContent("Hello");
    chat1.updateCurrentAIStream("Hi there");

    QJsonObject json = chat1.toJson();

    // Créer un nouveau chat et charger le JSON
    ChatImpl chat2(&llmservices);
    chat2.fromJson(json);

    QCOMPARE(chat2.getName(), QString("Original"));
    QCOMPARE(chat2.getCurrentApi(), QString("API"));
    QCOMPARE(chat2.getStreamed(), false);
    QCOMPARE(chat2.historyList().size(), 2);
    QCOMPARE(chat2.historyList().at(1).toMap()["role"].toString(), QString("assistant"));
    QCOMPARE(chat2.historyList().at(1).toMap()["content"].toString(), QString("Hi there"));
}

void ChatTest::test_export_helpers()
{
    qDebug() << "LLMServicesTest::test_export_helpers()";
    LLMServices llmservices(nullptr);
    ChatImpl chat(&llmservices);
    
    // Remplissage manuel de l'historique pour l'export (ou via simulation)
    chat.updateContent("Q1");
    chat.updateCurrentAIStream("R1");
    // Finaliser le stream pour que ça passe dans l'historique proprement
    // Dans le code actuel, finalizeStream est appelé au prochain updateContent ou via signal externe
    // On peut forcer l'ajout dans l'historique pour le test
    
    QString full = chat.getFullConversation();
    QVERIFY(full.contains("USER"));
    QVERIFY(full.contains("Q1"));
    
    QVERIFY(chat.getUserPrompts().contains("Q1"));
}

void ChatTest::test_finalize_stream()
{
    qDebug() << "LLMServicesTest::test_finalize_stream()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "Mock", LLMEnum::LLMType::LlamaCpp);
    llmservices.addAPI(mock);
    
    ChatImpl chat(&llmservices);
    chat.updateContent("Question");
    chat.updateCurrentAIStream("Partial answer");
    
    // Vérifier que le stream est en cours
    QCOMPARE(chat.historyList().last().toMap()["content"].toString(), QString("Partial answer"));
    
    // Finaliser le stream
    chat.updateContent("Next question"); // Cela devrait finaliser le stream précédent
    
    // Vérifier que l'historique contient bien deux questions et une réponse finalisée et une réponse ouverte
    QCOMPARE(chat.historyList().size(), 4);
    QCOMPARE(chat.historyList().at(1).toMap()["content"].toString(), QString("Partial answer"));
}

void ChatTest::test_error_handling()
{
    qDebug() << "LLMServicesTest::test_error_handling()";
    LLMServices llmservices(nullptr);
    ChatImpl chat(&llmservices);
    
    // Test setModel avec modèle inexistant
    chat.setModel("NonExistentModel:");
    QVERIFY(chat.getCurrentModel() == "none");
    
    // Test setApi avec API inexistant
    chat.setApi("NonExistentAPI");
    QVERIFY(chat.getCurrentApi() == "none");
}

void ChatTest::test_edge_cases()
{
    qDebug() << "LLMServicesTest::test_edge_cases()";
    LLMServices llmservices(nullptr);
    MockLLMService* mock = new MockLLMService(&llmservices, "Mock", LLMEnum::LLMType::LlamaCpp);
    llmservices.addAPI(mock);
    
    ChatImpl chat(&llmservices);
    
    // Test avec contenu vide
    chat.updateContent("");
    QCOMPARE(chat.historyList().last().toMap()["content"].toString(), "");
    
    // Test avec contenu très long
    chat.updateContent("Q");
    QString longContent = QString("A").repeated(10000);
    chat.updateCurrentAIStream(longContent); 
    QCOMPARE(chat.historyList().last().toMap()["content"].toString(), longContent);
    
    // Test avec caractères spéciaux
    chat.updateContent("Q");
    chat.updateCurrentAIStream("Réponse avec caractères spéciaux: éàèûç@#$%^&*()");
    QVERIFY(chat.historyList().last().toMap()["content"].toString().contains("éàèûç"));
}

QTEST_MAIN(ChatTest)
#include "tst_chat.moc"