#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QByteArray>
#include <QCborValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QTimer>
#include <QDebug>
#include <qcborvalue.h>

#include "Encryption.h"
#include "ErrorSystem.h"

#include "ChatStorageDistant.h"

static int ERRCODE_SQLDATABASE_NO_SERVER_URL = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_NO_SERVER_URL");
static int ERRCODE_SQLDATABASE_NO_ENCRYPTION_PASSWORD = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_NO_ENCRYPTION_PASSWORD");
static int ERRCODE_SQLDATABASE_REQUEST_TIMEOUT = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_REQUEST_TIMEOUT");
static int ERRCODE_SQLDATABASE_NO_USER_ID = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_NO_USER_ID");
static int ERRCODE_SQLDATABASE_NET_ERROR = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_NET_ERROR");
static int ERRCODE_SQLDATABASE_INVALID_RESPONSE = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_INVALID_RESPONSE");
static int ERRCODE_SQLDATABASE_SYNC_ERROR = ErrorSystem::instance().registerError("ERRCODE_SQLDATABASE_SYNC_ERROR");

// Test sans encryptage ni compression
static const bool ChatStorageEncryption = false;
static const bool ChatStorageCompression = false;

ChatStorageDistant::ChatStorageDistant(LLMServices* llmservices, const QString& serverUrl, 
                                const QString& userId, const QString& encryptionPassword) :
    ChatStorage(llmservices),
    networkManager_(new QNetworkAccessManager(this)),
    serverUrl_(!serverUrl.isEmpty() ? serverUrl : "http://127.0.0.1:8080/"),
    userId_(!userId.isEmpty() ? userId : "default"),
    encryptionPassword_(encryptionPassword),
    pendingLoadReply_(nullptr),
    pendingSaveReply_(nullptr),
    pendingSyncReply_(nullptr),
    pendingLoadSuccess_(false),
    pendingSaveSuccess_(false),
    pendingSyncSuccess_(false)
{
}

QUrl ChatStorageDistant::buildUrl(const QString& path) const
{
    QString base = serverUrl_;
    if (!base.endsWith('/'))
        base += '/';
    if (path.startsWith('/'))
        return QUrl(base + path.mid(1));
    return QUrl(base + path);
}

QNetworkRequest ChatStorageDistant::buildRequest(const QUrl& url) const
{
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!userId_.isEmpty())
        request.setRawHeader("X-User-Id", userId_.toUtf8());
    return request;
}

bool ChatStorageDistant::load(QList<Chat*>& chats)
{
    std::optional<QJsonArray> jsonArrayOpt = loadJson();
    if (jsonArrayOpt.has_value() && !jsonArrayOpt.value().isEmpty())
        return convertJsonToChatList(jsonArrayOpt.value(), chats, llmServices_);

    return false;
}

bool ChatStorageDistant::save(const QList<Chat*>& chats)
{
    QJsonArray jsonArray = convertChatListToJson(chats);
    return saveJson(jsonArray);
}

std::optional<QJsonArray> ChatStorageDistant::loadJson()
{
    if (serverUrl_.isEmpty())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NO_SERVER_URL);
        return std::nullopt;
    }

    if (ChatStorageEncryption && encryptionPassword_.isEmpty())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NO_ENCRYPTION_PASSWORD);
        return std::nullopt;
    }

    QUrl url = buildUrl("conversations");
    QUrlQuery query;
    if (!userId_.isEmpty())
        query.addQueryItem("user_id", userId_);
    url.setQuery(query);

    QNetworkRequest request = buildRequest(url);
    request.setRawHeader("X-User-Id", userId_.toUtf8());

    pendingLoadReply_ = networkManager_->get(request);
    pendingLoadSuccess_ = false;
    pendingLoadResult_ = QJsonArray();

    connect(pendingLoadReply_, &QNetworkReply::finished, this, &ChatStorageDistant::onLoadFinished);

    // Attendre la réponse de manière synchrone (pour compatibilité avec l'interface)
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(30000); // 30 secondes timeout

    connect(pendingLoadReply_, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start();
    loop.exec();

    if (!timeout.isActive())
    {
        // Timeout
        pendingLoadReply_->abort();
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_REQUEST_TIMEOUT);
        return std::nullopt;
    }

    if (!pendingLoadSuccess_)
        return std::nullopt;

    return pendingLoadResult_;
}

bool ChatStorageDistant::saveJson(const QJsonArray& chats)
{
    qDebug() << "ChatStorageDistant: saveJson";

    if (chats.isEmpty())
    {
        qDebug() << "No conversations to save";
        return true;
    }

    if (serverUrl_.isEmpty())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NO_SERVER_URL);
        return false;
    }

    if (ChatStorageEncryption && encryptionPassword_.isEmpty())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NO_ENCRYPTION_PASSWORD);
        return false;
    }

    QJsonArray preparedConversations;
    for (const QJsonValue& val : chats)
    {
        if (!val.isObject())
            continue;

        QJsonObject chat = val.toObject();
        QString id = chat["id"].toString();
        
        if (id.isEmpty())
            continue;

        QByteArray binaryData;

        // Créer un array avec un seul chat
        QJsonArray jsonArray;
        jsonArray.append(chat);
        if (ChatStorageEncryption)
        {
            binaryData = Encryption::encrypt(jsonArray, encryptionPassword_);
            if (binaryData.isEmpty())
            {
                qWarning() << "ChatStorageDistant::save: encryption failed for chat" << id;
                continue;
            }
        }
        else        
        {
            // On transforme le tableau JSON en valeur CBOR et on exporte directement en binaire (QByteArray)
            QCborValue cborValue = QCborValue::fromJsonValue(jsonArray);            
            binaryData = cborValue.toCbor();
        }        

        if (ChatStorageCompression)
            QByteArray compressed = qCompress(binaryData);        
        
        QJsonObject conv;
        conv["id"] = id;
        conv["user_id"] = userId_;
        conv["data"] = QString::fromLatin1(binaryData.toBase64());
        preparedConversations.append(conv);
    }

    if (preparedConversations.isEmpty())
    {
        qDebug() << "No conversations to save after encryption";
        return false;
    }

    // Utiliser l'endpoint de synchronisation batch
    return syncConversations(preparedConversations);
}

bool ChatStorageDistant::syncConversations(const QJsonArray& conversations)
{
    qDebug() << "ChatStorageDistant: syncConversations";

    if (serverUrl_.isEmpty())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NO_SERVER_URL);
        return false;
    }
    if (userId_.isEmpty())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NO_USER_ID);
        return false;
    }

    QJsonObject payload;
    payload["user_id"] = userId_;
    payload["conversations"] = conversations;

    QJsonDocument doc(payload);
    QByteArray data = doc.toJson();

    QUrl url = buildUrl("conversations/sync");
    QNetworkRequest request = buildRequest(url);

    pendingSyncReply_ = networkManager_->post(request, data);
    pendingSyncSuccess_ = false;

    connect(pendingSyncReply_, &QNetworkReply::finished, this, &ChatStorageDistant::onSyncFinished);

    // Attendre la réponse de manière synchrone
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    timeout.setInterval(30000);

    connect(pendingSyncReply_, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);

    timeout.start();
    loop.exec();

    if (!timeout.isActive())
    {
        pendingSyncReply_->abort();
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_REQUEST_TIMEOUT);
        return false;
    }

    return pendingSyncSuccess_;
}

void ChatStorageDistant::onLoadFinished()
{
    if (!pendingLoadReply_)
        return;

    if (pendingLoadReply_->error() != QNetworkReply::NoError)
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NET_ERROR, { pendingLoadReply_->errorString() });
        pendingLoadSuccess_ = false;
        pendingLoadReply_->deleteLater();
        pendingLoadReply_ = nullptr;
        return;
    }

    QByteArray responseData = pendingLoadReply_->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    
    if (doc.isNull() || !doc.isObject())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_INVALID_RESPONSE);
        pendingLoadSuccess_ = false;
        pendingLoadReply_->deleteLater();
        pendingLoadReply_ = nullptr;
        return;
    }

    QJsonObject response = doc.object();
    QJsonArray responseArray = response["conversations"].toArray();

    // Déchiffrer chaque conversation
    QJsonArray conversations;
    for (const QJsonValue& val : responseArray)
    {
        if (!val.isObject())
            continue;

        QJsonObject conv = val.toObject();

        QByteArray data;
        
        // Décompresser
        if (ChatStorageCompression)
        {
            QByteArray compressedData = QByteArray::fromBase64(conv["data"].toString().toLatin1());
            if (compressedData.isEmpty())
                continue;             
            data = qUncompress(compressedData);
        }
        else
        {
            data = QByteArray::fromBase64(conv["data"].toString().toLatin1());
        }
        
        if (data.isEmpty())
            continue;

        QJsonArray jsonArray;

        if (ChatStorageEncryption)
        {
            jsonArray = Encryption::decrypt(data, encryptionPassword_);        
        }   
        else     
        {
            QCborValue cborFromBinary = QCborValue::fromCbor(data);
            QJsonValue jsonValue = cborFromBinary.toJsonValue();
            jsonArray = jsonValue.toArray();
        }
        
        if (!jsonArray.isEmpty())
        {
            // Ajouter les métadonnées (id, timestamps) au premier élément
            for (const QJsonValue& chatVal : jsonArray)
            {
                if (chatVal.isObject())
                {
                    QJsonObject chat = chatVal.toObject();
                    // S'assurer que l'ID correspond
                    if (chat["id"].toString() == conv["id"].toString())                    
                        conversations.append(chat);                    
                }
            }
        }
    }

    pendingLoadResult_ = conversations;
    pendingLoadSuccess_ = true;
    pendingLoadReply_->deleteLater();
    pendingLoadReply_ = nullptr;
}

void ChatStorageDistant::onSyncFinished()
{
    qDebug() << "ChatStorageDistant: onSyncFinished";

    if (!pendingSyncReply_)
        return;

    if (pendingSyncReply_->error() != QNetworkReply::NoError)
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_NET_ERROR, { pendingSyncReply_->errorString() });
        pendingSyncSuccess_ = false;
        pendingSyncReply_->deleteLater();
        pendingSyncReply_ = nullptr;
        return;
    }

    QByteArray responseData = pendingSyncReply_->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    
    if (doc.isNull() || !doc.isObject())
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_INVALID_RESPONSE);
        pendingSyncSuccess_ = false;
        pendingSyncReply_->deleteLater();
        pendingSyncReply_ = nullptr;
        return;
    }

    QJsonObject response = doc.object();
    int saved = response["saved"].toInt();
    int errors = response["errors"].toInt();

    if (errors > 0)
    {
        ErrorSystem::instance().log(ERRCODE_SQLDATABASE_SYNC_ERROR, { QString("Sync completed with %1 errors").arg(errors) });
    }

    pendingSyncSuccess_ = (saved > 0);
    pendingSyncReply_->deleteLater();
    pendingSyncReply_ = nullptr;
}

void ChatStorageDistant::onSaveFinished()
{
    // Non utilisé pour l'instant (on utilise syncConversations)
    if (pendingSaveReply_)
    {
        pendingSaveReply_->deleteLater();
        pendingSaveReply_ = nullptr;
    }
}
