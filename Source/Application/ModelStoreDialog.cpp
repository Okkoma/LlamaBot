#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>

#include "ModelStoreDialog.h"

ModelStoreDialog::ModelStoreDialog(QObject* parent) :
    QObject(parent), 
    currentSort_(ModelSource::SortOrder::Trending),
    currentSizeFilter_(ModelSource::SizeFilter::All), 
    isDownloading_(false), 
    downloadProgress_(0.0f)
{
    initializeSources();
    
    // Set default source
    if (!sources_.empty())
    {
        currentSourceName_ = "Ollama"; // Default preference
        if (sources_.find(currentSourceName_) == sources_.end())
            currentSourceName_ = sources_.begin()->first;        
    }
}

ModelStoreDialog::~ModelStoreDialog()
{
}

void ModelStoreDialog::initializeSources()
{
    // Create all sources
    QStringList sources = ModelSource::getSources();
    for (const QString& source : sources)
        sources_.emplace(source, ModelSource::createModelSource(this, source));
    
    // Connect signals for all sources
    for (auto& source : sources_)
    {
        connect(source.second.get(), &ModelSource::downloadProgress, this, 
            [this](qint64 received, qint64 total) 
            {
                if (total > 0)
                {
                    downloadProgress_ = static_cast<float>(received) / total;
                    emit downloadProgressChanged();
                    
                    QString progressStr = QString("%1 / %2 MB").arg(received / 1024 / 1024).arg(total / 1024 / 1024);
                    setStatus("Downloading... " + progressStr);
                }
            });

        connect(source.second.get(), &ModelSource::downloadFinished, this, 
            [this](bool success, const QString& message) 
            {
                isDownloading_ = false;
                emit downloadingChanged();
                
                if (success)
                {
                    setStatus("Saved to " + message);
                    emit downloadFinished(true);
                }
                else
                {
                    setStatus("Download failed: " + message);
                    emit errorOccurred(message);
                    emit downloadFinished(false);
                }
            });
    }
}

QStringList ModelStoreDialog::availableSources() const
{
    QStringList keys;
    std::transform(
        sources_.begin(), sources_.end(),
        std::back_inserter(keys),
        [](const auto& pair) { return pair.first; }
    );
    return keys;
}

void ModelStoreDialog::setCurrentSource(const QString& sourceName)
{
    if (currentSourceName_ != sourceName && sources_.find(sourceName) != sources_.end())
    {
        currentSourceName_ = sourceName;
        emit currentSourceChanged();
        
        setStatus("Switched to " + sourceName);
    }
}

const QString& ModelStoreDialog::getCurrentSource() const
{
    return currentSourceName_;
}

void ModelStoreDialog::setStatus(const QString& message)
{
    if (statusMessage_ != message)
    {
        statusMessage_ = message;
        emit statusMessageChanged();
    }
}

void ModelStoreDialog::setAuthToken(const QString& token)
{
    auto it = sources_.find(currentSourceName_);
    QString currentToken = it->second->getAuthToken();
    if (token != currentToken)
    {
        it->second->setAuthToken(token);
        emit authTokenChanged();
    }
}

const QString& ModelStoreDialog::getAuthToken() const
{
    auto it = sources_.find(currentSourceName_);
    return it != sources_.end() ? it->second->getAuthToken() : NULL_QSTRING;
}

ModelSource::SortOrder ModelStoreDialog::parseSortOrder(const QString& sort)
{
    if (sort == "Trending") return ModelSource::SortOrder::Trending;
    if (sort == "Likes") return ModelSource::SortOrder::Likes;
    if (sort == "Date") return ModelSource::SortOrder::Date;
    return ModelSource::SortOrder::Trending;
}

ModelSource::SizeFilter ModelStoreDialog::parseSizeFilter(const QString& size)
{
    if (size == "2B") return ModelSource::SizeFilter::Size2B;
    if (size == "4B") return ModelSource::SizeFilter::Size4B;
    if (size == "8B") return ModelSource::SizeFilter::Size8B;
    if (size == "20B") return ModelSource::SizeFilter::Size20B;
    return ModelSource::SizeFilter::All;
}

void ModelStoreDialog::fetchModels()
{
    if (sources_.find(currentSourceName_) == sources_.end())
        return;

    setStatus("Fetching models from " + currentSourceName_ + "...");
    
    sources_[currentSourceName_]->fetchModels(currentSort_, currentSizeFilter_, searchName_,
        [this](bool success, const QVector<ModelManifest>& models, const QString& error)
        {
            if (success)
            {
                QVariantList list;
                for (const ModelManifest& model : models)                
                    list.append(modelToVariant(model));
                
                emit modelsListChanged(list);
                setStatus(QString("Found %1 models.").arg(list.size()));
            }
            else
            {
                setStatus("Error fetching models: " + error);
                emit modelsListChanged(QVariantList());
                emit errorOccurred(error);
            }
        });
}

void ModelStoreDialog::setSort(const QString& sortType)
{
    currentSort_ = parseSortOrder(sortType);
    fetchModels();
}

void ModelStoreDialog::setSizeFilter(const QString& size)
{
    currentSizeFilter_ = parseSizeFilter(size);
    fetchModels();
}

void ModelStoreDialog::setSearchName(const QString& name)
{
    searchName_ = name;
    fetchModels();
}

QVariantMap ModelStoreDialog::modelToVariant(const ModelManifest& model)
{
    QVariantMap map;
    map["name"] = model.name;
    map["date"] = model.date;
    map["trending"] = model.trending != -1 ? QString().setNum(model.trending) : "";
    map["likes"] = model.likes != -1 ? QString().setNum(model.likes) : "";
    map["downloads"] = model.downloads != -1 ? QString().setNum(model.downloads) : "";
    map["desc"] = model.desc;
    map["tags"] = model.tags;    
    map["size"] = model.size != 0 ? QString().setNum(model.size) : "";
    return map;
}

QVariantMap ModelStoreDialog::modelDetailsToVariant(const ModelDetails& modelDetails)
{
    QVariantMap map;
    map["createdDate"] = modelDetails.createdDate;
    map["updatedDate"] = modelDetails.updatedDate;
    map["license"] = modelDetails.license;
    map["languages"] = modelDetails.languages;
    map["digest"] = modelDetails.digest;
    map["size"] = modelDetails.maxSize != 0 ? QString().setNum(modelDetails.maxSize) : "";

    QVariantList files;
    for (const ModelFile& modelFile : modelDetails.files)
    {
        QVariantMap fileInfo;
        fileInfo["digest"] = modelFile.digest;
        fileInfo["name"] = modelFile.name;
        fileInfo["type"] = modelFile.type;
        files.append(fileInfo);
    }
    map["files"] = files;

    return map;
}

void ModelStoreDialog::fetchModelDetails(const QString& modelId)
{
    if (sources_.find(currentSourceName_) == sources_.end()) 
        return;
    
    lastModelId_ = modelId;
    setStatus("Fetching details for " + modelId + "...");
    
    sources_[currentSourceName_]->fetchModelDetails(modelId, 
        [this, modelId](bool success, const ModelDetails& modelDetails, const QString& error) {
            if (success)
            {
                QVariantMap details = modelDetailsToVariant(modelDetails);
                details["name"] = modelId; // Ensure name is available
                // Add description if available (not in manifest, maybe separate?)
                // For now, minimal info
                
                emit modelDetailsChanged(details);
                setStatus("Ready to download " + modelId);
            }
            else
            {
                setStatus("Error fetching details: " + error);
                emit errorOccurred(error);
            }
        });
}

void ModelStoreDialog::downloadFile(const QString& modelId, const QVariantMap& fileInfo)
{
    if (sources_.find(currentSourceName_) == sources_.end())
        return;
    
    ModelSource* source = sources_[currentSourceName_].get();

    // Determine save path
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists("models"))
        dir.mkpath("models");    
    QString savePath = dir.filePath("models/");

    isDownloading_ = true;
    downloadProgress_ = 0.0f;
    emit downloadingChanged();
    emit downloadProgressChanged();
    setStatus("Starting download...");
    
    source->downloadFile(modelId, fileInfo["digest"].toString(), fileInfo["name"].toString(), savePath);
}

void ModelStoreDialog::downloadAllFiles(const QString& modelId, const QVariantList& fileInfos)
{
    if (sources_.find(currentSourceName_) == sources_.end())
        return;

    ModelSource* source = sources_[currentSourceName_].get();

    // Determine save path
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists("models"))
        dir.mkpath("models");    
    QString savePath = dir.filePath("models/");

    isDownloading_ = true;
    downloadProgress_ = 0.0f;
    emit downloadingChanged();
    emit downloadProgressChanged();
    setStatus("Starting download...");

    for (const QVariant& fileInfo : fileInfos)
    {
        const QVariantMap& file = fileInfo.toMap();
        source->downloadFile(modelId, file["digest"].toString(), file["name"].toString(), savePath);
    }
}

void ModelStoreDialog::cancelDownload()
{
    if (sources_.find(currentSourceName_) != sources_.end())
    {
        sources_[currentSourceName_]->cancelDownload();
        isDownloading_ = false;
        emit downloadingChanged();
        setStatus("Download cancelled.");
    }
}
