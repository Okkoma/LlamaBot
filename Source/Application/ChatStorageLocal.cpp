#include <QDir>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>
#include <optional>

#include "ErrorSystem.h"
#include "LLMServices.h"

#include "ChatStorageLocal.h"


static int ERRCODE_SQLDATABASE_NO_DRIVER;
static int ERRCODE_SQLDATABASE_FAILED_OPEN;
static int ERRCODE_SQLDATABASE_FAILED_INITIALIZE;
static int ERRCODE_SQLDATABASE_FAILED_READ;
static int ERRCODE_SQLDATABASE_FAILED_TRANSACTION;
static int ERRCODE_SQLDATABASE_FAILED_DELETE;
static int ERRCODE_SQLDATABASE_FAILED_INSERT;
static int ERRCODE_SQLDATABASE_FAILED_COMMIT;

ChatStorageLocal::ChatStorageLocal(LLMServices* llmservices) :
    ChatStorage(llmservices),
    connectionName_(QString("chat_local_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
    ERRCODE_SQLDATABASE_NO_DRIVER = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_NO_DRIVER");
    ERRCODE_SQLDATABASE_FAILED_OPEN = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_OPEN");
    ERRCODE_SQLDATABASE_FAILED_INITIALIZE = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_INITIALIZE");
    ERRCODE_SQLDATABASE_FAILED_READ = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_READ");
    ERRCODE_SQLDATABASE_FAILED_TRANSACTION = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_TRANSACTION");
    ERRCODE_SQLDATABASE_FAILED_DELETE = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_DELETE");
    ERRCODE_SQLDATABASE_FAILED_INSERT = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_INSERT");
    ERRCODE_SQLDATABASE_FAILED_COMMIT = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_FAILED_COMMIT");
}

ChatStorageLocal::~ChatStorageLocal()
{
    if (db_.isValid())
        db_.close();
    if (!connectionName_.isEmpty())
        QSqlDatabase::removeDatabase(connectionName_);
}

QString ChatStorageLocal::dbPath() const
{
    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataLocation);
    if (!dir.exists())
        dir.mkpath(".");
    return dir.filePath("chat.db");
}

QString ChatStorageLocal::jsonfilePath() const
{
    const QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataLocation);
    if (!dir.exists())
        dir.mkpath(".");
    return dir.filePath("chats.json");    
}

bool ChatStorageLocal::isAvailable() const
{
    if (!QSqlDatabase::isDriverAvailable("QSQLITE"))
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_NO_DRIVER);
        qDebug() << "database: no sql driver !";
        return false;
    }

    return true;
}

bool ChatStorageLocal::openDatabase()
{
    if (!isAvailable())
        return false;

    if (db_.isValid() && db_.isOpen())
        return true;

    // Ouvrir la BDD
    db_ = QSqlDatabase::addDatabase("QSQLITE", connectionName_);
    db_.setDatabaseName(dbPath());
    if (!db_.open())
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_OPEN);
        qDebug() << "database: open failed !";
        return false;
    }

    // Initialiser le schema si la base est vide
    QSqlQuery q(db_);
    const char* createSql =
        "CREATE TABLE IF NOT EXISTS conversations ("
        "id TEXT PRIMARY KEY,"
        "name TEXT,"
        "payload_json TEXT NOT NULL,"
        "updated_at INTEGER NOT NULL"
        ");";
    if (!q.exec(createSql))
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_INITIALIZE, QStringList(q.lastError().text()));
        qDebug() << "database: initialization failed !";
        return false;
    }
    return true;
}

bool ChatStorageLocal::load(QList<Chat*>& chats)
{
    if (loadBinaryDb(chats))
        return true;

    std::optional<QJsonArray> jsonArrayOpt = loadJsonDb();
    if (!jsonArrayOpt.has_value() || jsonArrayOpt.value().isEmpty())
        jsonArrayOpt = loadJsonFile();

    if (jsonArrayOpt.has_value() && !jsonArrayOpt.value().isEmpty())
        return convertJsonToChatList(jsonArrayOpt.value(), chats, llmServices_);

    return false;
}

bool ChatStorageLocal::save(const QList<Chat*>& chats)
{
    if (saveBinaryDb(chats))
        return true;

    QJsonArray jsonArray = convertChatListToJson(chats);

    if (!saveJsonDb(jsonArray))
        return saveJsonFile(jsonArray);

    return true;
}

std::optional<QJsonArray> ChatStorageLocal::loadJsonDb()
{
    qDebug() << "ChatStorageLocal::loadJsonDb() ...";

    if (!QFile::exists(dbPath()) || !openDatabase())
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_OPEN, QStringList(dbPath()));            
        qDebug() << "ChatStorageLocal::loadJsonDb() ... ERRCODE_SQLDATABASE_FAILED_OPEN !";
        return std::nullopt;
    }

    QSqlQuery query(db_);
    if (!query.exec("SELECT payload_json FROM conversations ORDER BY updated_at ASC;"))
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_READ, QStringList(query.lastError().text()));            
        qDebug() << "ChatStorageLocal::loadJsonDb() ... ERRCODE_SQLDATABASE_FAILED_READ !";
        return std::nullopt;
    }

    QJsonArray array;
    while (query.next())
    {
        const QString payload = query.value(0).toString();
        const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
        if (doc.isObject())
            array.append(doc.object());
    }

    qDebug() << "ChatStorageLocal::loadJsonDb() ... OK !";
    return std::optional<QJsonArray>(array);
}

bool ChatStorageLocal::saveJsonDb(const QJsonArray& chats)
{
    if (!openDatabase())
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_OPEN);
        return false;
    }

    QSqlQuery query(db_);
    if (!db_.transaction())  
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_TRANSACTION, QStringList(db_.lastError().text()));
        return false;
    }

    // Stratégie simple pour l'instant: on remplace l'ensemble (facile et robuste).
    if (!query.exec("DELETE FROM conversations;"))
    {
        db_.rollback();
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_DELETE, QStringList(query.lastError().text()));
        return false;
    }

    query.clear();
    if (!query.prepare("INSERT INTO conversations(id, name, payload_json, updated_at) VALUES(?, ?, ?, ?);"))
    {
        db_.rollback();
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_INSERT, QStringList(query.lastError().text()));
        return false;
    }

    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const auto& v : chats)
    {
        if (!v.isObject())
            continue;

        const QJsonObject obj = v.toObject();
        const QString id = obj.value("id").toString();
        const QString name = obj.value("name").toString();
        const QString payload = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));

        query.bindValue(0, id);
        query.bindValue(1, name);
        query.bindValue(2, payload);
        query.bindValue(3, now);

        if (!query.exec())
        {
            db_.rollback();
            ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_INSERT, QStringList(query.lastError().text()));
            return false;
        }
    }

    if (!db_.commit())
    {
        ErrorSystem::instance().logError(ERRCODE_SQLDATABASE_FAILED_COMMIT, QStringList(db_.lastError().text()));
        return false;
    }

    qDebug() << "ChatStorageLocal::saveJsonDb() ... OK !";
    return true;
}

std::optional<QJsonArray> ChatStorageLocal::loadJsonFile()
{
    qDebug() << "ChatStorageLocal::loadJsonFile() ...";

    QFile file(jsonfilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        qDebug() << "ChatStorageLocal::loadJsonFile() ... failed to open " << file.fileName();
        return std::nullopt;
    }

    const QByteArray data = file.readAll();
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
    {
        qWarning() << "Invalid chats file format";
        return std::nullopt;
    }
    
    QJsonArray array = doc.array();
    qDebug() << "Chats loaded from local json file !";

    // Migration de json vers local db
    if (!array.isEmpty() && openDatabase())
        saveJsonDb(array);

    return std::optional<QJsonArray>(array);
}

bool ChatStorageLocal::saveJsonFile(const QJsonArray& chats)
{
    QFile file(jsonfilePath());
    if (file.open(QIODevice::WriteOnly))
    {
        QJsonDocument doc(chats);
        file.write(doc.toJson());
        file.close();
        qDebug() << "Chats saved to" << file.fileName();
    }
    else
    {
        qWarning() << "Could not open chats file for writing:" << file.fileName();
    }    
    return true;
}

bool ChatStorageLocal::loadBinaryDb(QList<Chat*>& chats)
{
    // TODO
    qDebug() << "ChatStorageLocal::loadBinaryDb to implement";    
    return false;
}

bool ChatStorageLocal::saveBinaryDb(const QList<Chat*>& chats)
{
    // TODO
    qDebug() << "ChatStorageLocal::saveBinaryDb to implement";    
    return false;
}