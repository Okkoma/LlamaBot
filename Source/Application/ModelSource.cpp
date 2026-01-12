#include "ModelSource.h"

std::unordered_map<QString, ModelSource::ModelSourceFactory> ModelSource::factories_;

ModelSource::ModelSource(QObject* parent) :
    manager_(new QNetworkAccessManager(this))
{
}

QVector<ModelManifest> ModelSource::filterBySize(const QVector<ModelManifest>& models, SizeFilter sizeFilter)
{
    if (sizeFilter == SizeFilter::All)
        return models;

    // Define size ranges (in bytes, approximate)
    qint64 maxSize = LLONG_MAX;

    switch (sizeFilter)
    {
    case SizeFilter::Size2B:
        maxSize = 2LL * (1L << 9);
        break;
    case SizeFilter::Size4B:
        maxSize = 4LL * (1L << 9);
        break;
    case SizeFilter::Size8B:
        maxSize = 8LL * (1L << 9);
        break;
    case SizeFilter::Size20B:
        maxSize = 20LL * (1L << 9);
        break;
    default:
        return models;
    }

    QVector<ModelManifest> filtered;
    for (const ModelManifest& model : models)
    {
        if (model.size <= maxSize)
            filtered.append(model);
    }

    return filtered;
}

QVector<ModelManifest> ModelSource::filterByGGUF(const QVector<ModelManifest>& models)
{
    QVector<ModelManifest> filtered;
    for (const ModelManifest& model : models)
    {
        // Check if the model name or tag indicates GGUF format
        if (model.name.endsWith(".gguf", Qt::CaseInsensitive) || 
            model.tags.contains("gguf", Qt::CaseInsensitive) ||
            model.desc.contains("gguf", Qt::CaseInsensitive))
        {
            filtered.append(model);
        }
    }

    return filtered;
}

void ModelSource::sortModels(QVector<ModelManifest>& models, SortOrder sort)
{
    std::sort(models.begin(), models.end(),
        [sort](const ModelManifest& a, const ModelManifest& b)
        {
            switch (sort)
            {
            case SortOrder::Trending:
                return a.trending < b.trending; 
            case SortOrder::Likes:
                return a.likes < b.likes;
            case SortOrder::Date:
                 return a.date < b.date;
            default:
                return a.name < b.name;
            }
        });
}

void ModelSource::downloadFileInternal(const QUrl& url, const QString& saveFullPath)
{
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    if (!authToken_.isEmpty())
        request.setRawHeader("Authorization", QString("Bearer %1").arg(authToken_).toUtf8());

    QFile* file = new QFile(saveFullPath);
    if (!file->open(QIODevice::WriteOnly))
    {
        emit downloadFinished(false, "Could not open file for writing");
        delete file;
        return;
    }

    int index = downloadInfos_.size();
    QNetworkReply* reply = manager_->get(request);
    downloadInfos_.emplace_back(file, reply);
    DownloadInfo& info = downloadInfos_.last();

    connect(reply, &QNetworkReply::downloadProgress, this, &ModelSource::downloadProgress);

    connect(reply, &QNetworkReply::readyRead, this,
        [this, file, reply]()
        {
            if (file && file->isOpen())            
                file->write(reply->readAll());            
        });

    connect(reply, &QNetworkReply::finished, this,
        [this, index, saveFullPath]()
        {
            if (downloadInfos_[index].file_)
            {
                downloadInfos_[index].file_->close();
                delete downloadInfos_[index].file_;
                downloadInfos_[index].file_ = nullptr;
            }

            int count = 0;
            for (DownloadInfo& info : downloadInfos_)
                if (!info.file_)
                    count++;  

            if (downloadInfos_[index].reply_->error() == QNetworkReply::NoError)
            {            
                if (count >= downloadInfos_.size())
                {
                    cancelDownload();
                    emit downloadFinished(true, saveFullPath);
                }
            }
            else
            {
                emit downloadFinished(false, downloadInfos_[index].reply_->errorString());
                cancelDownload();
                QFile::remove(saveFullPath);                
            }
        });
}

void ModelSource::cancelDownload()
{
    for (DownloadInfo& info : downloadInfos_)
    {
        if (info.reply_)
        {
            info.reply_->deleteLater();
            info.reply_->abort();
        }
        if (info.file_)
            delete info.file_;
    }
    downloadInfos_.clear();
}