#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QIcon>
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
    qDebug() << "LlamaBot - initialize ...";
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

    // Initialize QML
    qDebug() << "LlamaBot - initialize ... ui";
    qmlEngine_ = new QQmlApplicationEngine(this);

    qDebug() << "LlamaBot - initialize ... services";
    services_.initialize();

    qDebug() << "LlamaBot - initialize ... qml services";
    ChatController* controller = qmlEngine_->singletonInstance<ChatController*>("LlamaBotQml", "ChatController");
    qmlEngine_->singletonInstance<Clipboard*>("LlamaBotQml", "Clipboard");
    qmlEngine_->singletonInstance<ModelStore*>("LlamaBotQml", "ModelStore");
    qmlEngine_->singletonInstance<ThemeManager*>("LlamaBotQml", "ThemeManager");
    // Initialise ChatController et le lier avec LLMServices
    controller->initialize(ApplicationServices::get<LLMServices>());  

    // Charger l'interface principale Main.qml (QML module)
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
    qDebug() << "LlamaBot - initialize ... load main";
    qmlEngine_->loadFromModule("LlamaBotQml", "Main");
}

Application::~Application()
{
    if (qmlEngine_)
        delete qmlEngine_;
}
