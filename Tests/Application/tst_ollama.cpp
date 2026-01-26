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

    ChatImpl chat(&services);
    chat.setApi("Ollama");

    service->post(&chat, "Bonjour", true);
    QCOMPARE(chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString(), QString("Bonjour"));

    // attendre la réponse
    QSignalSpy spy(&chat, &Chat::messagesChanged);

    qDebug() << "OllamaTest::test_ollama_streaming() role: " << chat.data(chat.rowCount()-1, Chat::MessageRole::Role).toString();
    qDebug() << "OllamaTest::test_ollama_streaming() content: " << chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString();
    QVERIFY(chat.data(chat.rowCount()-1, Chat::MessageRole::Role).toString() == "assistant");
    QVERIFY(chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString().isEmpty() == false);
}

QTEST_MAIN(OllamaTest)
#include "tst_ollama.moc"
