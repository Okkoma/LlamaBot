#pragma once

#include <QDir>
#include <QFile>
#include <QJSValue>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QQmlEngine>
#include <QStandardPaths>
#include <functional>

struct ModelManifestLayer
{
    QString mediaType;
    qint64 size;
    QString digest;
};

struct ModelManifest
{
    int schemaVersion;
    QString mediaType;
    QVector<ModelManifestLayer> layers;
    ModelManifestLayer config;
};

struct LibraryModel
{
    QString name;
    QString description; // Not provided by api/tags, will generate or leave empty
    QString tag;
    QString digest;
    qint64 size;
};

class OllamaModelStoreDialog : public QObject
{
    Q_OBJECT

public:
    explicit OllamaModelStoreDialog(QObject* parent = nullptr);
    ~OllamaModelStoreDialog();

    void setQmlEngine(QQmlEngine* engine) { m_qmlEngine = engine; }

    // Fetch manifest for a model (e.g., "llama3", "phi3:latest")
    // C++ version
    void fetchManifest(const QString& modelName,
        std::function<void(bool success, const ModelManifest& manifest, const QString& error)> callback);

    // QML version with JS callback
    Q_INVOKABLE void fetchManifest(const QString& modelName, QJSValue callback);

    // Q_INVOKABLE for QML to trigger fetch
    Q_INVOKABLE void fetchLibraryQml()
    {
        fetchLibrary(
            [](bool, const QVector<LibraryModel>&, const QString&)
            {
            });
    };

    // Download a specific layer (blob) to a destination path (Low level)
    Q_INVOKABLE void downloadLayer(const QString& modelName, const QString& digest, const QString& savePath);

    // High level download: resolves persistence path automaticallly
    Q_INVOKABLE void downloadModel(const QString& modelName, const QString& digest);

    // Cancel current operation
    Q_INVOKABLE void cancelDownload();

signals:
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(bool success, const QString& errorOrPath);
    void libraryFetched(QVariantList models); // Signal for QML
    void libraryFetchError(QString error);    // Signal for QML error

private:
    QNetworkAccessManager* m_manager;
    QNetworkReply* m_currentDownloadReply;
    QFile* m_outputFile;
    QQmlEngine* m_qmlEngine = nullptr;

    void fetchLibrary(std::function<void(bool, const QVector<LibraryModel>&, const QString&)> callback);
    QString parseModelName(const QString& input, QString& tag);
    void handleManifestResponse(
        QNetworkReply* reply, std::function<void(bool, const ModelManifest&, const QString&)> callback);
};
