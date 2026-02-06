#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>

#include "ChatDbManager.h"


ChatDbManager::ChatDbManager(QObject* parent) :
    QObject(parent),
    connectionName_(QString("chatstore_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

ChatDbManager::~ChatDbManager()
{
    if (db_.isOpen())
        db_.close();
    QSqlDatabase::removeDatabase(connectionName_);
}

bool ChatDbManager::connect(const QString& host, int port, const QString& dbName,
                              const QString& user, const QString& password)
{
    db_ = QSqlDatabase::addDatabase("QPSQL", connectionName_);
    db_.setHostName(host);
    db_.setPort(port);
    db_.setDatabaseName(dbName);
    db_.setUserName(user);
    db_.setPassword(password);

    if (!db_.open())
    {
        qCritical() << "Erreur de connexion PostgreSQL:" << db_.lastError().text();
        return false;
    }

    qInfo() << "Connexion PostgreSQL établie:" << host << ":" << port << "/" << dbName;
    return true;
}

bool ChatDbManager::initializeSchema()
{
    return createTables();
}

bool ChatDbManager::createTables()
{
    QSqlQuery query(db_);

    // 1. Création de la table (sans les lignes INDEX)
    QString createTable = R"(
        CREATE TABLE IF NOT EXISTS conversations (
            id UUID PRIMARY KEY,
            user_id VARCHAR(255) NOT NULL,
            encrypted_data TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            deleted_at TIMESTAMP NULL
        )
    )";

    if (!query.exec(createTable))
    {
        qCritical() << "Erreur création table:" << query.lastError().text();
        return false;
    }

    // 2. Création des index (commandes séparées)
    // PostgreSQL ignore le "IF NOT EXISTS" sur les index dans les vieilles versions, 
    // mais c'est supporté depuis la v9.5+
    query.exec("CREATE INDEX IF NOT EXISTS idx_user_id ON conversations (user_id)");
    query.exec("CREATE INDEX IF NOT EXISTS idx_updated_at ON conversations (updated_at)");

    // Table: sync_metadata (pour la synchronisation)
    // Note : REFERENCES conversations(id) crée automatiquement une contrainte de clé étrangère
    QString createSyncTable = R"(
        CREATE TABLE IF NOT EXISTS sync_metadata (
            conversation_id UUID PRIMARY KEY REFERENCES conversations(id) ON DELETE CASCADE,
            last_sync_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            sync_state VARCHAR(50) DEFAULT 'synced',
            version INTEGER DEFAULT 1
        )
    )";

    if (!query.exec(createSyncTable))
    {
        qCritical() << "Erreur création table sync_metadata:" << query.lastError().text();
        return false;
    }

    // Création de l'index sur l'état de synchronisation
    // Très utile avec beaucoup de données et avec filtrage par état
    if (!query.exec("CREATE INDEX IF NOT EXISTS idx_sync_state ON sync_metadata (sync_state)"))
    {
        qCritical() << "Erreur création index idx_sync_state:" << query.lastError().text();
        return false;
    }

    qInfo() << "Schéma de base de données initialisé";
    return true;
}

QString ChatDbManager::extractUserId(const QHttpServerRequest& request) const
{
    // Essayer d'abord les query params
    auto queryParams = request.query();
    if (queryParams.hasQueryItem("user_id"))
        return queryParams.queryItemValue("user_id");

    // Sinon, essayer le header X-User-Id
    auto headers = request.headers();
    if (headers.contains("X-User-Id"))
        return QString::fromUtf8(headers.value("X-User-Id"));

    return {};
}

bool ChatDbManager::isValidUuid(const QString& id) const
{
    QUuid uuid(id);
    return !uuid.isNull();
}

QHttpServerResponse ChatDbManager::listConversations(const QHttpServerRequest& request)
{
    QString userId = extractUserId(request);
    if (userId.isEmpty())
    {
        return QHttpServerResponse("Missing user_id", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlQuery query(db_);
    query.prepare(R"(
        SELECT id, encrypted_data, created_at, updated_at
        FROM conversations
        WHERE user_id = :user_id AND deleted_at IS NULL
        ORDER BY updated_at DESC
    )");
    query.bindValue(":user_id", userId);

    if (!query.exec())
    {
        qWarning() << "Erreur SQL listConversations:" << query.lastError().text();
        return QHttpServerResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonArray conversations;
    while (query.next())
    {
        QJsonObject conv;
        conv["id"] = query.value("id").toString();
        conv["data"] = query.value("encrypted_data").toString();
        conv["created_at"] = query.value("created_at").toString();
        conv["updated_at"] = query.value("updated_at").toString();
        conversations.append(conv);
    }

    QJsonObject response;
    response["conversations"] = conversations;
    response["count"] = conversations.size();

    QJsonDocument doc(response);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse ChatDbManager::getConversation(const QString& id, const QHttpServerRequest& request)
{
    if (!isValidUuid(id))
    {
        return QHttpServerResponse("Invalid UUID", QHttpServerResponse::StatusCode::BadRequest);
    }

    QString userId = extractUserId(request);
    if (userId.isEmpty())
    {
        return QHttpServerResponse("Missing user_id", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlQuery query(db_);
    query.prepare(R"(
        SELECT id, encrypted_data, created_at, updated_at
        FROM conversations
        WHERE id = :id AND user_id = :user_id AND deleted_at IS NULL
    )");
    query.bindValue(":id", id);
    query.bindValue(":user_id", userId);

    if (!query.exec())
    {
        qWarning() << "Erreur SQL getConversation:" << query.lastError().text();
        return QHttpServerResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (!query.next())
    {
        return QHttpServerResponse("Not found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonObject response;
    response["id"] = query.value("id").toString();
    response["data"] = query.value("encrypted_data").toString();
    response["created_at"] = query.value("created_at").toString();
    response["updated_at"] = query.value("updated_at").toString();

    QJsonDocument doc(response);
    return QHttpServerResponse("application/json", doc.toJson());
}

QHttpServerResponse ChatDbManager::saveConversation(const QHttpServerRequest& request)
{
    QJsonDocument doc = QJsonDocument::fromJson(request.body());
    if (doc.isNull() || !doc.isObject())
    {
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    QString id = obj["id"].toString();
    QString userId = obj["user_id"].toString();
    QString encryptedData = obj["data"].toString();

    if (id.isEmpty() || userId.isEmpty() || encryptedData.isEmpty())
    {
        return QHttpServerResponse("Missing required fields", QHttpServerResponse::StatusCode::BadRequest);
    }

    if (!isValidUuid(id))
    {
        return QHttpServerResponse("Invalid UUID", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlQuery query(db_);
    query.prepare(R"(
        INSERT INTO conversations (id, user_id, encrypted_data, updated_at)
        VALUES (:id, :user_id, :encrypted_data, CURRENT_TIMESTAMP)
        ON CONFLICT (id) DO UPDATE SET
            encrypted_data = EXCLUDED.encrypted_data,
            updated_at = CURRENT_TIMESTAMP
    )");
    query.bindValue(":id", id);
    query.bindValue(":user_id", userId);
    query.bindValue(":encrypted_data", encryptedData);

    if (!query.exec())
    {
        qWarning() << "Erreur SQL saveConversation:" << query.lastError().text();
        return QHttpServerResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonObject response;
    response["id"] = id;
    response["status"] = "saved";

    QJsonDocument responseDoc(response);
    return QHttpServerResponse("application/json", responseDoc.toJson(), QHttpServerResponse::StatusCode::Created);
}

QHttpServerResponse ChatDbManager::updateConversation(const QString& id, const QHttpServerRequest& request)
{
    if (!isValidUuid(id))
    {
        return QHttpServerResponse("Invalid UUID", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonDocument doc = QJsonDocument::fromJson(request.body());
    if (doc.isNull() || !doc.isObject())
    {
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    QString encryptedData = obj["data"].toString();

    if (encryptedData.isEmpty())
    {
        return QHttpServerResponse("Missing data", QHttpServerResponse::StatusCode::BadRequest);
    }

    QString userId = extractUserId(request);
    if (userId.isEmpty())
    {
        return QHttpServerResponse("Missing user_id", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlQuery query(db_);
    query.prepare(R"(
        UPDATE conversations
        SET encrypted_data = :encrypted_data, updated_at = CURRENT_TIMESTAMP
        WHERE id = :id AND user_id = :user_id AND deleted_at IS NULL
    )");
    query.bindValue(":id", id);
    query.bindValue(":user_id", userId);
    query.bindValue(":encrypted_data", encryptedData);

    if (!query.exec())
    {
        qWarning() << "Erreur SQL updateConversation:" << query.lastError().text();
        return QHttpServerResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    if (query.numRowsAffected() == 0)
    {
        return QHttpServerResponse("Not found", QHttpServerResponse::StatusCode::NotFound);
    }

    QJsonObject response;
    response["id"] = id;
    response["status"] = "updated";

    QJsonDocument responseDoc(response);
    return QHttpServerResponse("application/json", responseDoc.toJson());
}

QHttpServerResponse ChatDbManager::deleteConversation(const QString& id, const QHttpServerRequest& request)
{
    if (!isValidUuid(id))
    {
        return QHttpServerResponse("Invalid UUID", QHttpServerResponse::StatusCode::BadRequest);
    }

    QString userId = extractUserId(request);
    if (userId.isEmpty())
    {
        return QHttpServerResponse("Missing user_id", QHttpServerResponse::StatusCode::BadRequest);
    }

    QSqlQuery query(db_);
    query.prepare(R"(
        UPDATE conversations
        SET deleted_at = CURRENT_TIMESTAMP
        WHERE id = :id AND user_id = :user_id
    )");
    query.bindValue(":id", id);
    query.bindValue(":user_id", userId);

    if (!query.exec())
    {
        qWarning() << "Erreur SQL deleteConversation:" << query.lastError().text();
        return QHttpServerResponse("Database error", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonObject response;
    response["id"] = id;
    response["status"] = "deleted";

    QJsonDocument responseDoc(response);
    return QHttpServerResponse("application/json", responseDoc.toJson());
}

QHttpServerResponse ChatDbManager::syncConversations(const QHttpServerRequest& request)
{
    QJsonDocument doc = QJsonDocument::fromJson(request.body());
    if (doc.isNull() || !doc.isObject())
    {
        qDebug() << "Invalid JSON";
        return QHttpServerResponse("Invalid JSON", QHttpServerResponse::StatusCode::BadRequest);
    }

    QJsonObject obj = doc.object();
    QString userId = obj["user_id"].toString();
    QJsonArray conversations = obj["conversations"].toArray();

    if (userId.isEmpty())
    {
        qDebug() << "Missing user_id";
        return QHttpServerResponse("Missing user_id", QHttpServerResponse::StatusCode::BadRequest);
    }

    db_.transaction();

    int saved = 0;
    int errors = 0;

    for (const QJsonValue& val : conversations)
    {
        if (!val.isObject())
            continue;

        QJsonObject conv = val.toObject();
        QString id = conv["id"].toString();
        QString encryptedData = conv["data"].toString();

        if (id.isEmpty() || encryptedData.isEmpty() || !isValidUuid(id))
        {
            errors++;
            continue;
        }

        // Création d'un SAVEPOINT pour isoler cette insertion
        // Cela permet de continuer la transaction même si cette requête échoue
        QSqlQuery savepointQuery(db_);
        savepointQuery.exec("SAVEPOINT sp_insert_item");

        QSqlQuery query(db_);
        query.prepare(
            "INSERT INTO conversations (id, user_id, encrypted_data, updated_at)"
            " VALUES (?, ?, ?, CURRENT_TIMESTAMP)"
            " ON CONFLICT (id) DO UPDATE SET"
            " encrypted_data = EXCLUDED.encrypted_data,"
            " updated_at = CURRENT_TIMESTAMP"
        );
        query.addBindValue(id);
        query.addBindValue(userId);
        query.addBindValue(encryptedData);

        if (query.exec())
        {
            savepointQuery.exec("RELEASE SAVEPOINT sp_insert_item");
            saved++;
        }
        else
        {
            qCritical() << "Erreur insertion conversation" << id << ":" << query.lastError().text();
            // Restauration de l'état avant le point de sauvegarde pour ne pas invalider toute la transaction
            savepointQuery.exec("ROLLBACK TO SAVEPOINT sp_insert_item");
            errors++;
        }
    }

    if (!db_.commit())
    {
        qDebug() << "Transaction failed";
        db_.rollback();
        return QHttpServerResponse("Transaction failed", QHttpServerResponse::StatusCode::InternalServerError);
    }

    QJsonObject response;
    response["saved"] = saved;
    response["errors"] = errors;
    QJsonDocument responseDoc(response);

    QHttpServerResponse serverResponse("application/json", responseDoc.toJson(QJsonDocument::Compact));

    qDebug() << "syncConversations:" << serverResponse.statusCode() << serverResponse.data();

    return serverResponse;
}
