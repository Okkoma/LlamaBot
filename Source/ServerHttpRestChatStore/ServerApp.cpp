#include <QCoreApplication>
#include <QHttpServer>
#include <QTcpServer>
#include <QHostAddress>
#include <QDebug>

#include "ChatDbManager.h"

#include "ServerApp.h"


ServerApp::ServerApp(QObject* parent) :
    QObject(parent),
    httpServer_(nullptr),
    chatDbManager_(nullptr)
{
    qInfo() << "ServerChatStore ...";
}

ServerApp::~ServerApp()
{
    if (chatDbManager_)
        delete chatDbManager_;

    if (httpServer_)
        delete httpServer_;
}

QString ServerApp::getHttpInfo() const
{
    QString info;
    QList<QTcpServer *> servers = httpServer_->servers();

    for (auto server : servers)
    {
        info += "adr: " + server->serverAddress().toString();
        info += " port: " + QString("%1").arg(server->serverPort());
    }
    return info;
}

QString ServerApp::getDatabaseInfo() const
{
    // TODO
    return {};
}

bool ServerApp::initializeDatabase(const QString& dbHost, int dbPort, const QString& dbName, const QString& dbUser, const QString& dbPassword)
{
    qInfo() << "ServerChatStore ... initializeDatabase ...";

    chatDbManager_ = new ChatDbManager(this);
    if (!chatDbManager_->connect(dbHost, dbPort, dbName, dbUser, dbPassword))
    {
        qCritical() << "Impossible de se connecter à PostgreSQL";
        return false;
    }

    if (!chatDbManager_->initializeSchema())
    {
        qCritical() << "Impossible d'initialiser le schéma de base de données";
        return false;
    }

    qInfo() << "ServerChatStore ... initializeDatabase ... ok";

    return true;
}

void ServerApp::setRoutes()
{
    // Route: GET /conversations - Liste toutes les conversations d'un utilisateur
    httpServer_->route("/conversations", QHttpServerRequest::Method::Get,
        [this](const QHttpServerRequest& request)
        {
            return chatDbManager_->listConversations(request);
        });

    // Route: GET /conversations/{id} - Récupère une conversation spécifique
    httpServer_->route("/conversations/<arg>", QHttpServerRequest::Method::Get,
        [this](const QString& id, const QHttpServerRequest& request)
        {
            return chatDbManager_->getConversation(id, request);
        });

    // Route: POST /conversations - Crée ou met à jour une conversation
    httpServer_->route("/conversations", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request)
        {
            return chatDbManager_->saveConversation(request);
        });

    // Route: PUT /conversations/{id} - Met à jour une conversation existante
    httpServer_->route("/conversations/<arg>", QHttpServerRequest::Method::Put,
        [this](const QString& id, const QHttpServerRequest& request)
        {
            return chatDbManager_->updateConversation(id, request);
        });

    // Route: DELETE /conversations/{id} - Supprime une conversation
    httpServer_->route("/conversations/<arg>", QHttpServerRequest::Method::Delete,
        [this](const QString& id, const QHttpServerRequest& request)
        {
            return chatDbManager_->deleteConversation(id, request);
        });

    // Route: POST /conversations/sync - Synchronisation batch (plusieurs conversations)
    httpServer_->route("/conversations/sync", QHttpServerRequest::Method::Post,
        [this](const QHttpServerRequest& request)
        {
            return chatDbManager_->syncConversations(request);
        });
}

bool ServerApp::start(int httpPort)
{
    httpServer_ = new QHttpServer(this);
    QTcpServer* tcpserver = new QTcpServer();
    if (!tcpserver->listen(QHostAddress::Any,httpPort) || !httpServer_->bind(tcpserver))
    {
        qCritical() << "Impossible de démarrer le serveur HTTP sur le port" << httpPort;
        delete tcpserver;
        return false;
    }

    setRoutes();

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("ServerChatStore");
    app.setApplicationVersion("0.1");

    QCommandLineParser parser;
    parser.setApplicationDescription("Serveur de stockage de conversations avec cryptage bout en bout");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(QCommandLineOption("port", "Port d'écoute du serveur HTTP", "port", "8080"));
    parser.addOption(QCommandLineOption("db-host", "Hôte de la base de données PostgreSQL", "host", "localhost"));
    parser.addOption(QCommandLineOption("db-port", "Port de la base de données PostgreSQL", "port", "5432"));
    parser.addOption(QCommandLineOption("db-user", "Utilisateur PostgreSQL", "user", "llamabot"));
    parser.addOption(QCommandLineOption("db-password", "Mot de passe PostgreSQL", "password", "%LlamaBot4321$"));  
    parser.addOption(QCommandLineOption("db-name", "Nom de la base de données PostgreSQL", "name", "llamabot_base"));
    parser.process(app);

    ServerApp server(&app);

    if (!server.initializeDatabase(parser.value("db-host"), parser.value("db-port").toInt(), 
                                parser.value("db-name"), parser.value("db-user"), parser.value("db-password")))
    {
        qCritical() << "Échec de l'initialisation de la base de données";
        return 1;
    }

    if (!server.start(parser.value("port").toInt()))
    {
        qCritical() << "Échec du démarrage du serveur HTTP";
        return 1;
    }

    qInfo() << "Http Server:" << server.getHttpInfo();
    qInfo() << "DataBase:" << server.getDatabaseInfo();

    return app.exec();
}