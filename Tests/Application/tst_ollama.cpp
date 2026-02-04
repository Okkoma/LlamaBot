#include <QtTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include "mock_services.h"

#include "../../Source/Application/OllamaService.h"
#include "../../Source/Application/ChatImpl.h"

class OllamaTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_ollama_service();
    void test_ollama_parsing();
    void test_ollama_streaming();
};

void OllamaTest::initTestCase()
{
    qDebug() << "OllamaTest::initTestCase()";
    ApplicationServices mockservice(this);
    mockservice.initialize(); 
    
    // Supprimer le fichier de config au cas où pour démarrer propre
    if (QFile::exists("LLMService.json"))
        QFile::remove("LLMService.json");

    LLMService::registerService<OllamaService>(LLMEnum::LLMType::Ollama);
}

void OllamaTest::test_ollama_service()
{    
    qDebug() << "OllamaTest::test_ollama_service()";
    LLMServices services(this);
    LLMService* service = services.get("Ollama");
    
    QVERIFY(service != nullptr);
    QCOMPARE(service->name_, QString("Ollama"));
}

void OllamaTest::test_ollama_parsing()
{
    qDebug() << "OllamaTest::test_ollama_parsing()";
    LLMServices services(this);
    LLMService* service = services.get("Ollama");

    ChatImpl chat(&services);
    chat.setApi("Ollama");
    chat.updateContent("Bonjour");

    // Simulate LLMServices::receive callback with raw Ollama /api/chat chunks
    QByteArray chunk1 = "{\"message\": {\"role\": \"assistant\", \"content\": \"Hello\"}, \"done\": false}\n";
    QByteArray chunk2 = "{\"message\": {\"role\": \"assistant\", \"content\": \" World\"}, \"done\": true}\n";
    
    services.receive(nullptr, &chat, chunk1);
    services.receive(nullptr, &chat, chunk2);
    
    QCOMPARE(chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString(), QString("Hello World"));
}

void OllamaTest::test_ollama_streaming()
{
    qDebug() << "OllamaTest::test_ollama_streaming()";
    LLMServices services(this);
    LLMService* service = services.get("Ollama");
    
    service->start();

    // Wait for service to be ready
    QTest::qWait(3000);
    QVERIFY(service->isReady() == true);

    ChatImpl chat(&services);
    chat.setApi("Ollama");

    QSignalSpy processingFinished(&chat, &ChatImpl::streamFinishedSignal);
    service->post(&chat, "Bonjour", true);
    qDebug() << "OllamaTest::test_ollama_streaming() ... posted";

    // Wait with event loop processing to allow cross-thread signals to be delivered
    bool waitForProcessingFinished = QTest::qWaitFor([&]() { return processingFinished.count() > 0; }, 10000);
    QVERIFY(waitForProcessingFinished == true);
    QCOMPARE(chat.data(0, Chat::MessageRole::Content).toString(), QString("Bonjour"));
    QVERIFY(chat.data(1, Chat::MessageRole::Role).toString() != "user");
    QVERIFY(chat.data(1, Chat::MessageRole::Content).toString().isEmpty() != true);
}

QTEST_MAIN(OllamaTest)
#include "tst_ollama.moc"
