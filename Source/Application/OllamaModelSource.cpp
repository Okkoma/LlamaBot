#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

#include "OllamaModelSource.h"
#include "LLMServiceDefs.h"

QVector<LLMModel> OllamaModelSource::cloudModels_;

const QString rx = "[ ,:\\-]";
const QStringList sizeFilterTagsStr[] =
{
    {""},

    {rx+"0.5b", rx+"1b", rx+"2b"},

    {rx+"0.5b", rx+"1b", rx+"2b", rx+"3b", rx+"4b"},

    {rx+"0.5b", rx+"1b", rx+"2b", rx+"3b", rx+"4b",rx+"5b", rx+"6b", rx+"7b", rx+"8b"},

    {rx+"0.5b", rx+"1b", rx+"2b", rx+"3b", rx+"4b",rx+"5b", rx+"6b", rx+"7b", rx+"8b",
         rx+"10b", rx+"12b", rx+"14b", rx+"16b",rx+"18b", rx+"20b"}
};

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

void OllamaModelSource::getOllamaCloudModels(std::vector<LLMModel>& models)
{
    for (LLMModel& cloudmodel : cloudModels_)
    {
        QString cloudmodelname = cloudmodel.toString();
        if (std::find_if(models.begin(),models.end(), 
            [cloudmodelname](LLMModel& localmodel) { return cloudmodelname == localmodel.toString(); })
                == models.end())
            models.push_back(cloudmodel);
    }
}

void OllamaModelSource::fetchModels(SortOrder sort, SizeFilter sizeFilter, const QString& searchName,
                                    std::function<void(bool, const QVector<ModelManifest>&, const QString&)> fetchCallback)
{
    QUrl url("https://ollama.com/api/tags");
    QNetworkRequest request(url);
    QNetworkReply* reply = manager_->get(request);

    cloudModels_.clear();

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, sort, sizeFilter, fetchCallback]()
        {
            if (reply->error() != QNetworkReply::NoError)
            {
                QString err = QString("Network Error: %1").arg(reply->errorString());
                fetchCallback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
            {
                QString err = "Invalid JSON response";
                fetchCallback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QJsonObject root = doc.object();
            QJsonArray modelsArray = root["models"].toArray();

            auto models = std::make_shared<QVector<ModelManifest>>();
            auto remaining = std::make_shared<int>(modelsArray.size());
            for (const QJsonValue& val : modelsArray)
            {
                QJsonObject modelObj = val.toObject();
                ModelManifest m;
                m.size = modelObj["size"].toVariant().toLongLong();
                m.name = modelObj["name"].toString();
                m.date = modelObj["modified_at"].toString().left(10);
                m.tags = modelObj["digest"].toString();
                m.desc = " ";

                fetchModelDetails(m.name,
                    [this, reply, sort, sizeFilter, remaining, m, models, fetchCallback](bool result, const ModelDetails& details, const QString& err)
                    {
                        bool cloudModel = true;
                        if (result)
                        {                            
                            for (const ModelFile& file : details.files)
                            {
                                if (file.type.contains("model"))
                                {
                                    qDebug() << "add gguf model:" << file.name;
                                    models->append(m);
                                    cloudModel = false;
                                    break;
                                }
                            }
                        }
                        if (cloudModel)
                        {
                            qDebug() << "add cloud model:" << m.name;      
                            LLMModel cloudModel;
                            cloudModel.name_ = m.name;
                            cloudModel.num_params_ = "cloud";                            
                            this->cloudModels_.append(cloudModel);
                        }

                        (*remaining)--;
                        if (*remaining == 0)
                        {
                            // Apply size filter
                            if (sizeFilter != SizeFilter::All)
                                (*models) = filterByTag(*models, sizeFilterTagsStr[(int)sizeFilter]);

                            // Note: Ollama API doesn't support custom sorting, so we use client-side sorting
                            this->sortModels(*models, sort);
                            fetchCallback(true, *models, "");
                            reply->deleteLater();
                        }
                    }
                );
            }
        }
    );
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
                reply->deleteLater();
                return;
            }
        
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
            {
                callback(false, {}, "Invalid JSON response");
                reply->deleteLater();
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
                    details.maxSize = size;
                    details.digest = file.name;
                }
            }

            callback(true, details, "");
            reply->deleteLater();
        }
    );
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
