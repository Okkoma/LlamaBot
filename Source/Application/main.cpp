#include <QDebug>
#include <QLoggingCategory>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlDebuggingEnabler>

#include "Application.h"

QtMessageHandler originalHandler = nullptr;

// Define the app log category
Q_DECLARE_LOGGING_CATEGORY(app)
Q_LOGGING_CATEGORY(app, "app")

// Define the test log category
Q_DECLARE_LOGGING_CATEGORY(test)
Q_LOGGING_CATEGORY(test, "test")

static FILE* logfile_;

void logToFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    if (logfile_ != nullptr)
    {
        QString message = qFormatLogMessage(type, context, msg);
        fprintf(logfile_, "%s\n", qPrintable(message));
        fflush(logfile_);
    }

    if (originalHandler)
        (*originalHandler)(type, context, msg);
}

int main(int argc, char * argv[])
{
    logfile_ = fopen("LlamaBot.txt", "w");
#ifdef LLAMABOT_QML_DEBUG
    // Enable QML debugging and set a specific port
    QQmlDebuggingEnabler::enableDebugging(true);
    qputenv("QML_DEBUG", "1");
    qputenv("QML_DEBUG_PORT", "10001");
#endif
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
	// Install custom message handler
	originalHandler = qInstallMessageHandler(logToFile);
    // Install custom message pattern
    qSetMessagePattern("(%{time yyyy-MM-dd hh:mm:ss,zzz})-[%{type}][%{category}] %{message}");

	qDebug() << "Debug message with category";
	qCDebug(app) << "Debug message with category";
	qCDebug(test) << "Debug message with category";

    Application app(argc, argv);
	return app.exec();
}
