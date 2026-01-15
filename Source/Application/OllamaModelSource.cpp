#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <qnetworkreply.h>

#include "OllamaModelSource.h"

OllamaModelSource::OllamaModelSource(QObject* parent) : 
    ModelSource(parent)
{
}

OllamaModelSource::~OllamaModelSource()
{
    cancelDownload();
}

QString OllamaModelSource::parseModelName(const QString& input, QString& tag)
{
    QString name = input;
    tag = "latest";
    if (name.contains(':'))
    {
        QStringList parts = name.split(':');
        name = parts[0];
        tag = parts[1];
    }

    // Handle library namespace (default to "library" if missing)
    if (!name.contains('/'))
        name = "library/" + name;
    
    return name;
}

void OllamaModelSource::fetchModels(SortOrder sort, SizeFilter sizeFilter, const QString& searchName,
                                    std::function<void(bool, const QVector<ModelManifest>&, const QString&)> callback)
{
    QUrl url("https://ollama.com/api/tags");
    QNetworkRequest request(url);

    QNetworkReply* reply = manager_->get(request);

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, sort, sizeFilter, callback]()
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
                QString err = "Invalid JSON response";
                callback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QJsonObject root = doc.object();
            QJsonArray modelsArray = root["models"].toArray();
            QVector<ModelManifest> models;

            // 2. Append Dynamic Models from API
            for (const QJsonValue& val : modelsArray)
            {
                QJsonObject modelObj = val.toObject();
                ModelManifest m;
                m.size = modelObj["size"].toVariant().toLongLong();            
                m.name = modelObj["name"].toString();
                m.date = modelObj["modified_at"].toString().left(10);
                m.tags = modelObj["digest"].toString();
                m.desc = QString("%1 (%2) %3").arg(m.name)
                                                 .arg(modelObj["modified_at"].toString().left(10))
                                                 .arg(m.tags);
                models.append(m);
            }

            // Apply filters
            if (sizeFilter != SizeFilter::All)            
                models = filterBySize(models, sizeFilter);            

            // Note: Ollama API doesn't support custom sorting, so we use client-side sorting
            sortModels(models, sort);

            callback(true, models, "");
            reply->deleteLater();
        });
}

void OllamaModelSource::fetchModelDetails(const QString& modelId, 
        std::function<void(bool, const ModelDetails&, const QString&)> callback)
{
    QString tag;
    QString cleanName = parseModelName(modelId, tag);

    QUrl url(QString("https://registry.ollama.com/v2/%1/manifests/%2").arg(cleanName).arg(tag));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("Accept", "application/vnd.docker.distribution.manifest.v2+json, "
                                   "application/vnd.oci.image.manifest.v1+json");

    QNetworkReply* reply = manager_->get(request);

    connect(reply, &QNetworkReply::finished, this,
        [this, modelId, reply, callback]()
        {
            QByteArray data = reply->readAll();

            if (reply->error() != QNetworkReply::NoError)
            {
                QString detail = reply->errorString();
                QJsonDocument errorDoc = QJsonDocument::fromJson(data);
                if (errorDoc.isObject() && errorDoc.object().contains("errors"))
                {
                    QJsonArray errors = errorDoc.object()["errors"].toArray();
                    if (!errors.isEmpty())                    
                        detail += " | Server: " + errors[0].toObject()["message"].toString();                    
                }
                int httpCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                detail = QString("HTTP %1: %2").arg(httpCode).arg(detail);
        
                callback(false, {}, detail);
                return;
            }
        
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
            {
                callback(false, {}, "Invalid JSON response");
                return;
            }
        
            QJsonObject root = doc.object();
            ModelDetails details;

            QJsonObject configObj = root["config"].toObject();
            ModelFile file;
            file.type = configObj["mediaType"].toString();
            file.digest = configObj["digest"].toString();
            file.name = modelId + "-config.json";
            details.files.append(file);

            QJsonArray files = root["layers"].toArray();
            quint64 maxSize = 0U;
            for (const QJsonValue& val : files)
            {
                QJsonObject obj = val.toObject();
                ModelFile file;
                file.type = obj["mediaType"].toString(); 
                file.digest = obj["digest"].toString();                  
                if (file.type.contains("model"))
                    file.name = modelId + "-model.gguf";
                else if (file.type.contains("docker"))
                    file.name = modelId + "-docker.json";
                else if (file.type.contains("license"))
                    file.name = modelId + "-license.txt";
                else if (file.type.contains("template"))
                    file.name = modelId + "-template.json";
                else if (file.type.contains("params"))
                    file.name = modelId + "-params.json";
                else
                    file.name = modelId + "-" + file.type;    
                details.files.append(file);

                // get the maximum size of files
                quint64 size = obj["size"].toVariant().toULongLong(); 
                if (size > maxSize)
                {
                    maxSize = size;
                    details.digest = file.name;
                }
            }
        
            callback(true, details, "");
            reply->deleteLater();
        });
}

void OllamaModelSource::downloadFile(const QString& modelId, const QString& digest, 
                                     const QString& fileName, const QString& savePath)
{
    QString tag;
    QString cleanName = parseModelName(modelId, tag);
    QUrl url(QString("https://registry.ollama.com/v2/%1/blobs/%2").arg(cleanName).arg(digest));

    // Sanitize filename
    QString sanitizedFileName = fileName;
    sanitizedFileName.replace('/', '_');
    sanitizedFileName.replace(':', '_');

    downloadFileInternal(url, savePath + sanitizedFileName);
}
