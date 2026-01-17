#pragma once

#include <QObject>
#include <QMap>
#include <QVariant>
#include <memory>

#include "ModelSource.h"

/**
 * @brief Controller class for the Model Store Dialog
 * 
 * Manages multiple model sources and exposes them to QML.
 * Handles fetching models, filtering, sorting, and downloading.
 */
class ModelStoreDialog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availableSources READ availableSources CONSTANT)
    Q_PROPERTY(const QString& currentSource READ getCurrentSource WRITE setCurrentSource NOTIFY currentSourceChanged)
    Q_PROPERTY(const QString& authToken READ getAuthToken WRITE setAuthToken NOTIFY authTokenChanged)
    Q_PROPERTY(const QString& searchName READ getSearchName WRITE setSearchName NOTIFY searchNameChanged)    
    Q_PROPERTY(const QString& statusMessage READ getStatusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(bool isDownloading READ isDownloading NOTIFY downloadingChanged)
    Q_PROPERTY(float downloadProgress READ downloadProgress NOTIFY downloadProgressChanged)

public:
    explicit ModelStoreDialog(QObject* parent = nullptr);
    ~ModelStoreDialog();

    template <typename T> void RegisterModelSource(const QString& name);

    void setCurrentSource(const QString& sourceName);
    const QString& getCurrentSource() const;
    
    void setSearchName(const QString& name);
    const QString& getSearchName() const { return searchName_; }

    void setAuthToken(const QString& token);
    const QString& getAuthToken() const;

    QStringList availableSources() const;

    const QString getStatusMessage() const { return statusMessage_; }
    bool isDownloading() const { return isDownloading_; }
    float downloadProgress() const { return downloadProgress_; }

    Q_INVOKABLE void setSort(const QString& sortType);
    Q_INVOKABLE void setSizeFilter(const QString& size);

    Q_INVOKABLE void fetchModels();
    // Fetch details for a specific model ID
    Q_INVOKABLE void fetchModelDetails(const QString& modelId);
    
    // Download the currently selected/viewed model
    Q_INVOKABLE void downloadFile(const QString& modelId, const QVariantMap& fileInfo);
    Q_INVOKABLE void downloadAllFiles(const QString& modelId, const QVariantList& fileInfos);
    Q_INVOKABLE void cancelDownload();

signals:
    void currentSourceChanged();
    void modelsListChanged(QVariantList models);
    void modelDetailsChanged(QVariantMap details);
    void downloadingChanged();
    void statusMessageChanged();
    void downloadProgressChanged();
    void authTokenChanged();
    void searchNameChanged();
    void errorOccurred(QString error);
    void downloadFinished(bool success);

private:
    void initializeSources();
    ModelSource::SortOrder parseSortOrder(const QString& sort);
    ModelSource::SizeFilter parseSizeFilter(const QString& size);
    void setStatus(const QString& message);
    
    // Helper to convert C++ struct to QML-friendly map
    QVariantMap modelToVariant(const ModelManifest& model);
    QVariantMap modelDetailsToVariant(const ModelDetails& manifest);

    std::unordered_map<QString, std::unique_ptr<ModelSource> > sources_;

    ModelSource::SortOrder currentSort_;
    ModelSource::SizeFilter currentSizeFilter_;

    QString currentSourceName_;    
    QString searchName_;
    QString statusMessage_;

    bool isDownloading_;
    float downloadProgress_;
    
    // Temporary storage for model details to allow downloading
    QString lastModelId_;
};
