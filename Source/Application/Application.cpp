#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QQmlContext>

#include "ApplicationServices.h"
#include "LLMServices.h"

#include "ChatController.h"
#include "Clipboard.h"
#include "ModelStore.h"
#include "RAGService.h"
#include "ThemeManager.h"
#include "IconThemeImageProvider.h"

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

    // Enregistrement du provider d'icones
    qmlEngine_->addImageProvider(QStringLiteral("icon"), new IconThemeImageProvider);

    qDebug() << "LlamaBot - initialize ... services";
    services_.initialize();
    qmlEngine_->rootContext()->setContextProperty("LLMServices", services_.get<LLMServices>());
    qmlEngine_->rootContext()->setContextProperty("RAGService", services_.get<RAGService>());

    qDebug() << "LlamaBot - initialize ... qml services";
    qmlEngine_->singletonInstance<ChatController*>("LlamaBotQml", "ChatController");
    qmlEngine_->singletonInstance<Clipboard*>("LlamaBotQml", "Clipboard");
    qmlEngine_->singletonInstance<ModelStore*>("LlamaBotQml", "ModelStore");
    qmlEngine_->singletonInstance<ThemeManager*>("LlamaBotQml", "ThemeManager");

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
