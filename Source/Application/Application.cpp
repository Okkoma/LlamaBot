#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "LLMServices.h"

#include "ChatController.h"
#include "Clipboard.h"
#include "ModelStore.h"
#include "ThemeManager.h"

#include "Application.h"

Application::Application(int& argc, char** argv) :
    QApplication(argc, argv),
    services_(this)
{
    setApplicationName("LlamaBot");
    setApplicationVersion("0.1.0");

    // Set application icon
    QIcon appIcon("qrc:/icons/icon1.png");
    setWindowIcon(appIcon);
    setDesktopFileName("llamabot");

    QDir::setCurrent(applicationDirPath());

    QCommandLineParser parser;
    parser.setApplicationDescription("ChatBot QML Application");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*this);

    services_.initialize();

    // Initialize Clipboard
    clipboard_ = new Clipboard(this);

    // Initialize Controller
    chatController_ = new ChatController(ApplicationServices::get<LLMServices>(), this);

    // Initialize Model Store Dialog
    modelStore_ = new ModelStore(this);

    // Initialize QML
    qmlEngine_ = new QQmlApplicationEngine(this);

    // Register Controller and Types
    qmlEngine_->rootContext()->setContextProperty("app", this);

    // Load Main.qml from QML module
    connect(
        qmlEngine_, &QQmlApplicationEngine::objectCreated, this,
        [](QObject* obj, const QUrl& objUrl)
        {
            Q_UNUSED(objUrl);
            if (!obj)
            {
                qCritical() << "Failed to load Main.qml";
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection
    );

    // Charge le point d'entrée du module
    qmlEngine_->loadFromModule("LlamaBotQml", "Main");
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

ThemeManager* Application::themeManager() const
{
    return ApplicationServices::get<ThemeManager>();
}
