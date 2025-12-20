#include <QCommandLineParser>
#include <QDebug>
#include <QDir>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "Application.h"
#include "ChatController.h"
#include "LLMService.h"
#include "OllamaModelStoreDialog.h"
#include "ThemeManager.h"


Application::Application(int& argc, char** argv) :
    QApplication(argc, argv),
    qmlEngine_(nullptr),
    chatController_(nullptr),
    modelStoreDialog_(nullptr),
    services_(this)
{
    setApplicationName("ChatBot");
    setApplicationVersion("0.1.0");

    // Set application icon
    QIcon appIcon("qrc:/icons/icon1.png");
    setWindowIcon(appIcon);
    setDesktopFileName("chatbot");

    QDir::setCurrent(applicationDirPath());

    QCommandLineParser parser;
    parser.setApplicationDescription("ChatBot QML Application");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*this);

    // Initialize ThemeManager
    themeManager_ = new ThemeManager(this);

    // Initialize Controller
    chatController_ = new ChatController(ApplicationServices::get<LLMService>(), this);

    // Initialize Model Store Dialog
    modelStoreDialog_ = new OllamaModelStoreDialog(this);

    // Initialize QML
    qmlEngine_ = new QQmlApplicationEngine(this);

    // Set QML engine for model store
    modelStoreDialog_->setQmlEngine(qmlEngine_);

    // Register Controller and Types
    qmlEngine_->rootContext()->setContextProperty("chatController", chatController_);
    qmlEngine_->rootContext()->setContextProperty("ollamaModelStoreDialog", modelStoreDialog_);
    qmlEngine_->rootContext()->setContextProperty("application", this);
    qmlEngine_->rootContext()->setContextProperty("themeManager", themeManager_);

    // Load Main.qml
    const QUrl url(QStringLiteral("qrc:/ressources/Main.qml"));
    connect(
        qmlEngine_, &QQmlApplicationEngine::objectCreated, this,
        [url](QObject* obj, const QUrl& objUrl)
        {
            if (!obj && url == objUrl)
            {
                qCritical() << "Failed to load Main.qml";
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);

    qmlEngine_->load(url);
}

Application::~Application()
{
    // Critical: Destroy QML engine BEFORE services_ is destroyed
    // Otherwise QML will try to access destroyed C++ objects
    if (qmlEngine_)
    {
        // Clear the context property to prevent QML from accessing chatController during shutdown
        qmlEngine_->rootContext()->setContextProperty("chatController", QVariant());

        // Delete the engine explicitly
        delete qmlEngine_;
        qmlEngine_ = nullptr;
    }

    delete themeManager_;
    // chatController_ will be deleted automatically as it's parented to 'this'
    // services_ will be destroyed after this destructor completes
}
