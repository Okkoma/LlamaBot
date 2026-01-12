#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>
#include <QSettings>

#include "HuggingFaceModelSource.h"

HuggingFaceModelSource::HuggingFaceModelSource(QObject* parent) :
    ModelSource(parent)
{
    QSettings settings;
    authToken_ = settings.value("hfToken").toString();
}

HuggingFaceModelSource::~HuggingFaceModelSource()
{
    cancelDownload();
}

QString HuggingFaceModelSource::sortOrderToApiParam(SortOrder sort)
{
    switch (sort)
    {
    case SortOrder::Trending:
        return "trendingScore";
    case SortOrder::Likes:
        return "likes";
    case SortOrder::Date:
        return "createdAt";
    default:
        return "trendingScore";
    }
}

void HuggingFaceModelSource::fetchModels(SortOrder sort, SizeFilter sizeFilter, const QString& searchName,
                                          std::function<void(bool, const QVector<ModelManifest>&, const QString&)> callback)
{
    QUrl url("https://huggingface.co/api/models");
    QUrlQuery query;

    // Always Apply GGUF filter
    query.addQueryItem("filter", "gguf");

    if (!searchName.isEmpty())
        query.addQueryItem("filter", searchName);

    // Apply sorting
    query.addQueryItem("sort", sortOrderToApiParam(sort));
    query.addQueryItem("direction", "-1"); // Descending order
    query.addQueryItem("limit", "100");
    query.addQueryItem("private", "false");

    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    if (!authToken_.isEmpty())
        request.setRawHeader("Authorization", QString("Bearer %1").arg(authToken_).toUtf8());

    QNetworkReply* reply = manager_->get(request);

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, sizeFilter, callback]()
        {
            if (reply->error() != QNetworkReply::NoError)
            {
                QString err = QString("Network Error: %1").arg(reply->errorString());
                callback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isArray())
            {
                QString err = "Invalid JSON response (expected array)";
                callback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QJsonArray modelsArray = doc.array();
            QVector<ModelManifest> modelManifests;

            for (const QJsonValue& val : modelsArray)
            {
                QJsonObject modelObj = val.toObject();

                ModelManifest manifest;
                manifest.name = modelObj["id"].toString();
                manifest.date = modelObj["lastModified"].toString();
                manifest.trending = modelObj["trendingScore"].toVariant().toInt();
                manifest.likes = modelObj["likes"].toVariant().toInt();
                manifest.downloads = modelObj["downloads"].toVariant().toInt();
                QString pipeline = modelObj["pipeline_tag"].toString();
                if (!pipeline.isEmpty())
                    manifest.desc += "\npipeline: " + pipeline;
                manifest.desc += "\ncreated: " + modelObj["createdAt"].toString() + 
                                 " - updated: " + modelObj["lastModified"].toString();
                manifest.size = 0;
                modelManifests.append(manifest);
            }

            // Apply size filter
            if (sizeFilter != SizeFilter::All)
                modelManifests = filterBySize(modelManifests, sizeFilter);

            callback(true, modelManifests, "");
            reply->deleteLater();
        });
}

void HuggingFaceModelSource::fetchModelDetails(const QString& modelId, 
        std::function<void(bool, const ModelDetails&, const QString&)> callback)
{
    QUrl url(QString("https://huggingface.co/api/models/%1").arg(modelId));
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    if (!authToken_.isEmpty())
        request.setRawHeader("Authorization", QString("Bearer %1").arg(authToken_).toUtf8());

    QNetworkReply* reply = manager_->get(request);

    connect(reply, &QNetworkReply::finished, this,
        [reply, callback]()
        {
            if (reply->error() != QNetworkReply::NoError)
            {
                QString err = QString("Network Error: %1").arg(reply->errorString());
                callback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
            {
                callback(false, {}, "Invalid JSON response");
                reply->deleteLater();
                return;
            }

            QJsonObject modelInfo = doc.object();

            ModelDetails modelDetails;
            modelDetails.createdDate = modelInfo["createdAt"].toString();
            modelDetails.updatedDate = modelInfo["lastModified"].toString();
            modelDetails.digest= modelInfo["sha"].toString();

            QJsonObject cardData = modelInfo["cardData"].toObject();
            modelDetails.license = cardData["license"].toString();
            QJsonArray languages = cardData["language"].toArray();
            for (const QJsonValue& lang : languages)
                modelDetails.languages += lang.toString();

            if (modelInfo.contains("gguf"))
            {
                QJsonObject ggufInfo = modelInfo["gguf"].toObject();
                modelDetails.maxSize = ggufInfo["total"].toVariant().toLongLong();
            }
            // Extract GGUF files from siblings
            QJsonArray siblings = modelInfo["siblings"].toArray();
            for (const QJsonValue& sibling : siblings)
            {
                QJsonObject fileObj = sibling.toObject();
                QString filename = fileObj["rfilename"].toString();
                if (filename.endsWith(".gguf", Qt::CaseInsensitive))
                {
                    ModelFile file;
                    file.name = filename;
                    file.type = "gguf";
                    modelDetails.files.append(file);
                }
            }

            if (modelDetails.files.isEmpty())
            {
                callback(false, {}, "No GGUF files found for this model");
                reply->deleteLater();
                return;
            }

            callback(true, modelDetails, "");
            reply->deleteLater();
        });
}

void HuggingFaceModelSource::downloadFile(const QString& modelId, const QString& digest, 
                                          const QString& fileName, const QString& savePath)
{
    QUrl url(QString("https://huggingface.co/%1/resolve/main/%2").arg(modelId, fileName));

    // Sanitize filename
    QString sanitizedFileName = fileName;
    sanitizedFileName.replace('/', '_');
    sanitizedFileName.replace(':', '_');

    downloadFileInternal(url, savePath + sanitizedFileName);
}