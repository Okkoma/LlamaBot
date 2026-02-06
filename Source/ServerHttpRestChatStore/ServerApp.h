#pragma once

#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QHttpServer>
#include <QString>

class ChatDbManager;

/**
 * @brief Application serveur principale
 * 
 * Gère le serveur HTTP et la coordination entre les handlers et la base de données.
 */
class ServerApp : public QObject
{
    Q_OBJECT

public:
    explicit ServerApp(QObject* parent = nullptr);
    ~ServerApp() override;

    /**
     * @brief Initialise la connexion à la base de données PostgreSQL
     */
    bool initializeDatabase(const QString& dbHost, int dbPort, const QString& dbName, const QString& dbUser, const QString& dbPassword);

    /**
     * @brief Démarre le serveur HTTP
     */
    bool start(int port);

    QString getHttpInfo() const;
    QString getDatabaseInfo() const;

private:
    void setRoutes();

    QHttpServer* httpServer_;
    ChatDbManager* chatDbManager_;
    QCommandLineParser parser_;
};
