#include <QtTest>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>

#include "../Source/Application/LLMServices.h"
#include "../Source/Application/ChatImpl.h"

// Mock compatible avec la factory LLMService::createService
class MockService : public LLMService {
public:
    // Constructeur pour enregistrement manuel
    MockService(LLMServices* s, const QString& name, LLMEnum::LLMType type, bool ready = true) 
        : LLMService(static_cast<int>(type), s, name), ready_(ready) {}

    // Constructeur pour la factory
    MockService(LLMServices* s, const QVariantMap& params) 
        : LLMService(s, params), ready_(true) {}

    std::vector<LLMModel> getAvailableModels() const override {
        return models_;
    }

    void addModel(const QString& name, const QString& params = "7B") {
        LLMModel m;
        m.name_ = name;
        m.num_params_ = params;
        models_.push_back(m);
    }

    bool isReady() const override { return ready_; }
    void setReady(bool r) { ready_ = r; }

    std::vector<LLMModel> models_;
    bool ready_;
};

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
};

void LLMServicesTest::initTestCase()
{
    // Enregistrer le MockService pour tous les types
    LLMService::registerService<MockService>(static_cast<int>(LLMEnum::LLMType::LlamaCpp));
    LLMService::registerService<MockService>(static_cast<int>(LLMEnum::LLMType::Ollama));
    LLMService::registerService<MockService>(static_cast<int>(LLMEnum::LLMType::OpenAI));

    // Supprimer le fichier de config au cas où pour démarrer propre
    if (QFile::exists("LLMService.json"))
        QFile::remove("LLMService.json");
        
    qDebug() << "Démarrage des tests unitaires pour LLMServices";
}

void LLMServicesTest::cleanupTestCase()
{
    if (QFile::exists("LLMService.json"))
        QFile::remove("LLMService.json");
}

void LLMServicesTest::test_construction_and_defaults()
{
    LLMServices services(nullptr);
    // Comme on a enregistré les factory, createDefaultServiceJsonFile devrait avoir ajouté les APIs par défaut
    QVERIFY(services.getAPIs().size() >= 1); 
    QVERIFY(services.get("LlamaCpp") != nullptr);
}

void LLMServicesTest::test_api_management()
{
    LLMServices services(nullptr);
    
    // 1. Test de récupération par nom unique (pour éviter le conflit avec Ollama/LlamaCpp par défaut)
    MockService* m1 = new MockService(&services, "CustomAPI", LLMEnum::LLMType::LlamaCpp);
    services.addAPI(m1);
    QCOMPARE(services.get("CustomAPI"), (LLMService*)m1);

    // 2. Test de récupération par type (en utilisant OpenAI qui n'est pas ajouté par défaut)
    QString openAIName = enumValueToString<LLMEnum>("LLMType", LLMEnum::LLMType::OpenAI);
    MockService* m2 = new MockService(&services, openAIName, LLMEnum::LLMType::OpenAI, false);
    services.addAPI(m2);

    QCOMPARE(services.get(openAIName), (LLMService*)m2);
    QCOMPARE(services.get(LLMEnum::LLMType::OpenAI), (LLMService*)m2);
}

void LLMServicesTest::test_model_aggregation()
{
    LLMServices services(nullptr);
    MockService* m1 = new MockService(&services, "M1", LLMEnum::LLMType::LlamaCpp);
    m1->addModel("model-a", "7B");
    services.addAPI(m1);

    LLMModel model = services.getModel("model-a:7B");
    QCOMPARE(model.name_, QString("model-a"));
}

void LLMServicesTest::test_is_service_available()
{
    LLMServices services(nullptr);
    QString openAIName = enumValueToString<LLMEnum>("LLMType", LLMEnum::LLMType::OpenAI);
    MockService* m1 = new MockService(&services, openAIName, LLMEnum::LLMType::OpenAI, true);
    services.addAPI(m1);
    
    QVERIFY(services.isServiceAvailable(LLMEnum::LLMType::OpenAI));
    
    m1->setReady(false);
    QVERIFY(!services.isServiceAvailable(LLMEnum::LLMType::OpenAI));
}

void LLMServicesTest::test_receive_parsing()
{
    LLMServices services(nullptr);
    MockService* api = new MockService(&services, "ParserAPI", LLMEnum::LLMType::LlamaCpp);
    ChatImpl chat(&services);
    
    chat.updateContent("Trigger"); 
    
    QByteArray data = "{\"response\": \"Hello\"}\n{\"response\": \" World\"}";
    services.receive(api, &chat, data);
    
    QVERIFY(chat.historyList().last().toMap()["content"].toString().contains("Hello World"));
}

void LLMServicesTest::test_config_persistence()
{
    // Sauvegarder
    {
        LLMServices services(nullptr);
        MockService* m = new MockService(&services, "PersistAPI", LLMEnum::LLMType::OpenAI);
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

QTEST_MAIN(LLMServicesTest)
#include "tst_llmservices.moc"