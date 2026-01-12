#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "define.h"

struct ModelManifest
{
    QString name;
    QString date;
    int trending{-1};
    int likes{-1};
    int downloads{-1};
    QString desc;
    QString tags;
    quint64 size;
};

struct ModelFile
{
    QString name;
    QString digest;
    QString type;
};

struct ModelDetails
{
    QString createdDate;
    QString updatedDate;
    QString license;
    QStringList languages;
    QString digest;
    QVector<ModelFile> files;
    quint64 maxSize;
};

/**
 * @brief Abstract base class for model sources
 * 
 * Defines the interface that all model source implementations must follow.
 * Supports fetching models with filtering, sorting, and downloading.
 */
class ModelSource : public QObject
{
    Q_OBJECT

public:
    enum class SortOrder
    {
        Trending,
        Likes,
        Date,
    };
    Q_ENUM(SortOrder)

    enum class SizeFilter
    {
        All,
        Size2B,
        Size4B,
        Size8B,
        Size20B
    };
    Q_ENUM(SizeFilter)

    using ModelSourceFactory = std::function<std::unique_ptr<ModelSource>(QObject* parent)>;
    template <typename T>
    static void registerSource(const QString& name)
    {
        factories_[name] = [](QObject* parent)
        {
            return std::make_unique<T>(parent);
        };
    }

    static QStringList getSources()
    {
        QStringList keys;
        std::transform(
            factories_.begin(), factories_.end(),
            std::back_inserter(keys),
            [](const auto& pair) { return pair.first; }
        );
        return keys;
    }
    
    static std::unique_ptr<ModelSource> createModelSource(QObject* parent, const QString& name)
    {
        return factories_[name](parent);
    }

    explicit ModelSource(QObject* parent = nullptr);
    virtual ~ModelSource() = default;

    /**
     * @brief Fetch models from this source with filters applied
     * @param sort Sort order for results
     * @param sizeFilter Filter by model parameter size
     * @param searchName Filter by name
     * @param callback Callback with (success, models, error)
     */
    virtual void fetchModels(SortOrder sort, SizeFilter sizeFilter, const QString& searchName,
        std::function<void(bool, const QVector<ModelManifest>&, const QString&)> callback) = 0;

    /**
     * @brief Fetch detailed information about a specific model
     * @param modelId Unique identifier for the model (source-specific format)
     * @param callback Callback with (success, manifest, error)
     */
    virtual void fetchModelDetails(const QString& modelId,
        std::function<void(bool, const ModelDetails&, const QString&)> callback) = 0;

    /**
     * @brief Download a file to the specified path
     * @param modelId Unique identifier for the model
     * @param digest Unique digest
     * @param filename Unique filename 
     * @param savePath Destination path for the downloaded file
     */
    virtual void downloadFile(const QString& modelId, const QString& digest, 
                              const QString& fileName, const QString& savePath) = 0;

    /**
     * @brief Cancel any ongoing download operation
     */
    void cancelDownload();

    /**
     * @brief Get the human-readable name of this source
     */
    virtual QString sourceName() const = 0;

    void setAuthToken(const QString& token) { authToken_ = token; }
    const QString& getAuthToken() const { return authToken_; }

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& errorOrPath);

protected:
    /**
     * @brief Helper function to filter models by size
     * @param models List of models to filter
     * @param sizeFilter Size filter to apply
     * @return Filtered list of models
     */
    static QVector<ModelManifest> filterBySize(const QVector<ModelManifest>& models, SizeFilter sizeFilter);

    /**
     * @brief Helper function to filter models by GGUF format
     * @param models List of models to filter
     * @return Filtered list containing only GGUF models
     */
    static QVector<ModelManifest> filterByGGUF(const QVector<ModelManifest>& models);

    /**
     * @brief Helper function to sort models
     * @param models List of models to sort (sorted in-place)
     * @param sort Sort order to apply
     */
    static void sortModels(QVector<ModelManifest>& models, SortOrder sort);

    void downloadFileInternal(const QUrl& url, const QString& saveFullPath);

    QString authToken_;

    struct DownloadInfo
    {
        DownloadInfo(QFile* file, QNetworkReply* reply) : file_(file), reply_(reply) {}
    
        QFile* file_{nullptr};
        QNetworkReply* reply_{nullptr};
    };

    QVector<DownloadInfo> downloadInfos_;
    QNetworkAccessManager* manager_;

private:
    static std::unordered_map<QString, ModelSourceFactory> factories_;
};
