#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "ChatController.h"
#include "Clipboard.h"
#include "LLMServices.h"
#include "ModelStoreDialog.h"
#include "ThemeManager.h"

#include "Application.h"

Application::Application(int& argc, char** argv) :
    QApplication(argc, argv),
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

    // Initialize Clipboard
    clipboard_ = new Clipboard(this);

    // Initialize Controller
    chatController_ = new ChatController(ApplicationServices::get<LLMServices>(), this);

    // Initialize Model Store Dialog
    modelStoreDialog_ = new ModelStoreDialog(this);

    // Initialize QML
    qmlEngine_ = new QQmlApplicationEngine(this);

    // Register Controller and Types
    qmlEngine_->rootContext()->setContextProperty("chatController", chatController_);
    qmlEngine_->rootContext()->setContextProperty("modelStoreDialog", modelStoreDialog_);
    qmlEngine_->rootContext()->setContextProperty("application", this);
    qmlEngine_->rootContext()->setContextProperty("themeManager", themeManager_);
    qmlEngine_->rootContext()->setContextProperty("clipboard", clipboard_);

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
    if (qmlEngine_)
    {
        // Clear the context property to prevent QML from accessing chatController during shutdown
        qmlEngine_->rootContext()->setContextProperty("chatController", QVariant());

        // Delete the engine explicitly
        delete qmlEngine_;
        qmlEngine_ = nullptr;
    }
}
