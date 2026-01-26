#include <QtTest>

#include "mock_services.h"

#include "../../Source/Application/LlamaCppService.h"
#include "../../Source/Application/ChatImpl.h"

class LlamaCppTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_llamacpp_service();
    void test_llamacpp_parameters();
    void test_llamacpp_streaming();
};

void LlamaCppTest::initTestCase()
{
    qDebug() << "LlamaCppTest::initTestCase()";
    ApplicationServices mockservice(this);
    mockservice.initialize();       
    LLMService::registerService<LlamaCppService>(LLMEnum::LLMType::LlamaCpp);
}

void LlamaCppTest::test_llamacpp_service()
{    
    qDebug() << "LlamaCppTest::test_llamacpp_service()";
    LLMServices services(this);
    LLMService* service = services.get("LlamaCpp");
    
    QVERIFY(service != nullptr);
    QCOMPARE(service->name_, QString("LlamaCpp"));
    QVERIFY(service->isReady() == true);
}

void LlamaCppTest::test_llamacpp_parameters()
{
    qDebug() << "LlamaCppTest::test_llamacpp_parameters()";
    LLMServices services(nullptr);
    LlamaCppService* service = new LlamaCppService(&services, "LlamaCppParams");
    
    service->setDefaultGpuLayers(32);
    service->setDefaultContextSize(4096);
    service->setDefaultUseGpu(true);
    
    QCOMPARE(service->getGpuLayers(), 32);
    QCOMPARE(service->getContextSize(), 4096);
    QVERIFY(service->isUsingGpu() == true);
}

void LlamaCppTest::test_llamacpp_streaming()
{
    qDebug() << "LlamaCppTest::test_llamacpp_streaming()";
    LLMServices services(this);
    LLMService* service = services.get("LlamaCpp");
    
    ChatImpl chat(&services);
    chat.setApi("LlamaCpp");

    service->post(&chat, "Bonjour", true);
    QCOMPARE(chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString(), QString("Bonjour"));

    // attendre la réponse
    QSignalSpy spy(&chat, &Chat::messagesChanged);

    qDebug() << "LlamaCppTest::test_llamacpp_streaming() role: " << chat.data(chat.rowCount()-1, Chat::MessageRole::Role).toString();
    qDebug() << "LlamaCppTest::test_llamacpp_streaming() content: " << chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString();
    QVERIFY(chat.data(chat.rowCount()-1, Chat::MessageRole::Role).toString() == "assistant");
    QVERIFY(chat.data(chat.rowCount()-1, Chat::MessageRole::Content).toString().isEmpty() == false);
}

QTEST_MAIN(LlamaCppTest)
#include "tst_llamacpp.moc"
