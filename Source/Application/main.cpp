#include <QDebug>
#include <QLoggingCategory>

#include "Application.h"

QtMessageHandler originalHandler = nullptr;

// Define the app log category
Q_DECLARE_LOGGING_CATEGORY(app)
Q_LOGGING_CATEGORY(app, "app")

// Define the test log category
Q_DECLARE_LOGGING_CATEGORY(test)
Q_LOGGING_CATEGORY(test, "test")

void logToFile(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QString message = qFormatLogMessage(type, context, msg);
    static FILE *f = fopen("LlamaBot.txt", "w");
    fprintf(f, "%s\n", qPrintable(message));
    fflush(f);

    if (originalHandler)
        (*originalHandler)(type, context, msg);
}

int main(int argc, char * argv[])
{
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
