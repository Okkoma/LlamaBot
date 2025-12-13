#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QEvent>
#include <QKeyEvent>
#include <QProcess>
#include <QRegularExpression>
#include <QSettings>
#include <QStyle>

#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "Application.h"
#include "ChatController.h"
#include "LLMService.h"

// #include "MainWindow.h"

Application::Application(int& argc, char** argv) :
    QApplication(argc, argv), window_(nullptr), qmlEngine_(nullptr), chatController_(nullptr), services_(this)
{
    setApplicationName("ChatBot");
    setApplicationVersion("0.1.0");

    QDir::setCurrent(applicationDirPath());

    QCommandLineParser parser;
    parser.setApplicationDescription("ChatBot QML Application");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(*this);

    // Initialize Controller
    chatController_ = new ChatController(ApplicationServices::get<LLMService>(), this);

    // Initialize QML
    qmlEngine_ = new QQmlApplicationEngine(this);

    // Register Controller and Types
    qmlEngine_->rootContext()->setContextProperty("chatController", chatController_);

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

    // Legacy Widget Initialization (Disabled)
    /*
    window_ = new MainWindow();
    loadSettings();
    setFont(currentFont_);
    window_->show();
    */
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

    // chatController_ will be deleted automatically as it's parented to 'this'
    // services_ will be destroyed after this destructor completes
}

void Application::ApplyStyle(const QString& style)
{
    // Legacy support
    qDebug() << "Application::ApplyStyle deprecated:" << style;
}

void Application::setFont(const QFont& font)
{
    qDebug() << "Application::setFont deprecated:" << font;
    currentFont_ = font;
    QApplication::setFont(font);
}

void Application::setStyleName(const QString& styleName)
{
    currentStyleName_ = styleName;
}

QString Application::styleName() const
{
    return currentStyleName_;
}

void Application::saveSettings()
{
    // Port to QSettings compatible with QML if needed
}

void Application::loadSettings() {}

bool Application::eventFilter(QObject* obj, QEvent* event)
{
    return QApplication::eventFilter(obj, event);
}
