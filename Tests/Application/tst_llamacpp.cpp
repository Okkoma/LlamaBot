#include <QtTest>
#include <QElapsedTimer>
#include <QCoreApplication>

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

    // Supprimer le fichier de config au cas où pour démarrer propre
    if (QFile::exists("LLMService.json"))
        QFile::remove("LLMService.json");

    LLMService::registerService<LlamaCppService>(LLMEnum::LLMType::LlamaCpp);
}

void LlamaCppTest::test_llamacpp_service()
{    
    qDebug() << "LlamaCppTest::test_llamacpp_service()";
    LLMServices services(this);
    LLMService* service = services.get("LlamaCpp");

    if (!service)
        QSKIP("LlamaCppTest::test_llamacpp_service() no LlamaCpp service ... skipped"); 

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
    qDebug() << "LlamaCppTest::test_llamacpp_streaming() ...";
    LLMServices services(this);

    LLMService* llamaCppService = services.get("LlamaCpp");

    if (!llamaCppService)
        QSKIP("LlamaCppTest::test_llamacpp_streaming() no LlamaCpp service ... skipped");   

    llamaCppService->start();
    
    ChatImpl chat(&services);
    chat.setApi("LlamaCpp");

    // Check if has a model available
    // if no model, skip the test
    if (!llamaCppService->getAvailableModels().size())
        QSKIP("LlamaCppTest::test_llamacpp_streaming() no model ... skipped");

    // 1. use local model from the main application target
    // with LLMService::setCustomModelsPath
    // 2. or add a symbolic link to an available model 
    // in the local test target directory .local/share/Test_LlamaCpp
    // => Ok use this one.

    QSignalSpy processingFinished(&chat, &ChatImpl::processingFinished);

    llamaCppService->post(&chat, "Bonjour", true);
    
    qDebug() << "LlamaCppTest::test_llamacpp_streaming() ... posted";

    // Wait with event loop processing to allow cross-thread signals to be delivered
    QElapsedTimer timer;
    timer.start();
    const int timeoutMs = 10000; // 10 seconds timeout for model loading + generation
    
    while (processingFinished.count() == 0 && timer.elapsed() < timeoutMs)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(50);
    }

    if (processingFinished.count() > 0)
        qDebug() << "LlamaCppTest::test_llamacpp_streaming() ... generation finished successfully!";
    else
        qDebug() << "LlamaCppTest::test_llamacpp_streaming() ... test timeout !";

    qDebug() << "LlamaCppTest::test_llamacpp_streaming() [0] role: " << chat.data(0, Chat::MessageRole::Role).toString();
    qDebug() << "LlamaCppTest::test_llamacpp_streaming() [0] content: " << chat.data(0, Chat::MessageRole::Content).toString();
    qDebug() << "LlamaCppTest::test_llamacpp_streaming() [1] role: " << chat.data(1, Chat::MessageRole::Role).toString();
    qDebug() << "LlamaCppTest::test_llamacpp_streaming() [1] content: " << chat.data(1, Chat::MessageRole::Content).toString();

    QCOMPARE(chat.data(0, Chat::MessageRole::Content).toString(), QString("Bonjour"));
    QVERIFY(chat.data(1, Chat::MessageRole::Role).toString() != "user");
    QVERIFY(chat.data(1, Chat::MessageRole::Content).toString().isEmpty() != true);
}

QTEST_MAIN(LlamaCppTest)
#include "tst_llamacpp.moc"
