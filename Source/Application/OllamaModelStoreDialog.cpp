#include "OllamaModelStoreDialog.h"
#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>

OllamaModelStoreDialog::OllamaModelStoreDialog(QObject* parent) :
    QObject(parent), m_manager(new QNetworkAccessManager(this)), m_currentDownloadReply(nullptr), m_outputFile(nullptr)
{
}

OllamaModelStoreDialog::~OllamaModelStoreDialog()
{
    cancelDownload();
}

QString OllamaModelStoreDialog::parseModelName(const QString& input, QString& tag)
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
    {
        name = "library/" + name;
    }
    return name;
}

void OllamaModelStoreDialog::fetchManifest(
    const QString& modelName, std::function<void(bool, const ModelManifest&, const QString&)> callback)
{
    QString tag;
    QString cleanName = parseModelName(modelName, tag);

    QUrl url(QString("https://registry.ollama.com/v2/%1/manifests/%2").arg(cleanName, tag));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    // Accept header is important for Docker/OCI registry v2
    // Must include OCI specific headers as Ollama pivoted to OCI
    request.setRawHeader("Accept", "application/vnd.docker.distribution.manifest.v2+json, "
                                   "application/vnd.oci.image.manifest.v1+json");

    QNetworkReply* reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, callback]()
        {
            handleManifestResponse(reply, callback);
            reply->deleteLater();
        });
}

void OllamaModelStoreDialog::handleManifestResponse(
    QNetworkReply* reply, std::function<void(bool, const ModelManifest&, const QString&)> callback)
{
    QByteArray data = reply->readAll();

    if (reply->error() != QNetworkReply::NoError)
    {
        // Try to parse JSON error from body if available
        QString detail = reply->errorString();
        QJsonDocument errorDoc = QJsonDocument::fromJson(data);
        if (errorDoc.isObject() && errorDoc.object().contains("errors"))
        {
            QJsonArray errors = errorDoc.object()["errors"].toArray();
            if (!errors.isEmpty())
            {
                detail += " | Server: " + errors[0].toObject()["message"].toString();
            }
        }
        // Add HTTP status code
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
    ModelManifest manifest;
    manifest.schemaVersion = root["schemaVersion"].toInt();
    manifest.mediaType = root["mediaType"].toString();

    QJsonArray layers = root["layers"].toArray();
    for (const QJsonValue& val : layers)
    {
        QJsonObject layerObj = val.toObject();
        ModelManifestLayer layer;
        layer.mediaType = layerObj["mediaType"].toString();
        layer.size = layerObj["size"].toVariant().toLongLong();
        layer.digest = layerObj["digest"].toString();
        manifest.layers.append(layer);
    }

    QJsonObject configObj = root["config"].toObject();
    manifest.config.mediaType = configObj["mediaType"].toString();
    manifest.config.size = configObj["size"].toVariant().toLongLong();
    manifest.config.digest = configObj["digest"].toString();

    callback(true, manifest, "");
}

void OllamaModelStoreDialog::fetchManifest(const QString& modelName, QJSValue callback)
{
    // Capture callback by value (copying QJSValue is cheap/reference-like)
    QJSValue cb = callback; // Mutable copy

    fetchManifest(modelName,
        [this, cb](bool success, const ModelManifest& manifest, const QString& error) mutable
        {
            if (!cb.isCallable())
                return;

            // Use the injected engine or try to get one
            QJSEngine* engine = m_qmlEngine;
            if (!engine)
            {
                qWarning() << "OllamaModelStoreDialog: No QJSEngine set, cannot convert "
                              "objects for callback!";
                // Best effort fallback is usually impossible for objects without engine
                // We'll proceed but toScriptValue might fail or we skip complex args
            }

            QJSValueList args;
            args << success;

            if (success && engine)
            {
                // Convert Manifest to JS Object using engine
                QVariantMap m;
                m["mediaType"] = manifest.mediaType;
                m["schemaVersion"] = manifest.schemaVersion;

                QVariantList layers;
                for (const auto& l : manifest.layers)
                {
                    QVariantMap lm;
                    lm["digest"] = l.digest;
                    lm["mediaType"] = l.mediaType;
                    lm["size"] = l.size;
                    layers.append(lm);
                }
                m["layers"] = layers;

                // Config
                QVariantMap config;
                config["digest"] = manifest.config.digest;
                config["size"] = manifest.config.size;
                m["config"] = config;

                args << engine->toScriptValue(m);
                args << QJSValue(QJSValue::UndefinedValue); // No error
            }
            else
            {
                args << QJSValue(QJSValue::NullValue); // No manifest
                args << (engine ? engine->toScriptValue(error) : QJSValue(error));
            }

            cb.call(args);
        });
}

void OllamaModelStoreDialog::fetchLibrary(
    std::function<void(bool, const QVector<LibraryModel>&, const QString&)> callback)
{
    QUrl url("https://ollama.com/api/tags");
    QNetworkRequest request(url);

    QNetworkReply* reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this,
        [this, reply, callback]()
        {
            if (reply->error() != QNetworkReply::NoError)
            {
                QString err = QString("Network Error: %1").arg(reply->errorString());
                emit libraryFetchError(err);
                callback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
            {
                QString err = "Invalid JSON response";
                emit libraryFetchError(err);
                callback(false, {}, err);
                reply->deleteLater();
                return;
            }

            QJsonObject root = doc.object();
            QJsonArray modelsArray = root["models"].toArray();
            QVector<LibraryModel> models;
            QVariantList qmlList;

            // 1. Inject Essentials/Popular Models
            // (since api/tags is incomplete/recent-only)
            struct Featured
            {
                const char* name;
                const char* desc;
            };

            std::vector<Featured> featured = { { "llama3.2", "Meta's latest efficient model" },
                { "llama3.1", "Meta's powerful open model" }, { "mistral", "Mistral AI 7B model" },
                { "gemma2", "Google's open model" }, { "phi3.5", "Microsoft's small model" },
                { "llava", "Multi-modal (Vision) model" }, { "qwen2.5", "Alibaba's powerful model" },
                { "deepseek-v3", "DeepSeek's strong coding model" } };

            for (const auto& f : featured)
            {
                LibraryModel m;
                m.name = f.name;
                m.tag = f.name;
                m.digest = ""; // Unknown until fetch
                m.size = 1;    // Dummy size to pass filter
                m.description = QString("⭐ %1").arg(f.desc);
                models.append(m);

                QVariantMap vm;
                vm["name"] = m.name;
                vm["tag"] = m.tag;
                vm["digest"] = m.digest;
                vm["description"] = m.description;
                vm["size"] = m.size;
                qmlList.append(vm);
            }

            // 2. Append Dynamic Models from API
            for (const QJsonValue& val : modelsArray)
            {
                QJsonObject modelObj = val.toObject();
                LibraryModel m;
                m.size = modelObj["size"].toVariant().toLongLong();

                // FILTER: Skip "Cloud" or Massive models (> 100 GB) or
                // Invalid/Placeholder (<= 0) 100 GB = 100 * 1024 * 1024 * 1024 =
                // 107374182400 bytes
                if (m.size > 107374182400LL || m.size <= 0)
                {
                    continue;
                }

                m.name = modelObj["model"].toString(); // In api/tags, 'model' and 'name' seem identical
                m.tag = m.name;
                m.digest = modelObj["digest"].toString();
                m.description = QString("%1 (%2)").arg(m.name).arg(
                    modelObj["modified_at"].toString().left(10)); // Generate simple desc

                models.append(m);

                // Prepare QVariantMap for QML
                QVariantMap vm;
                vm["name"] = m.name;
                vm["tag"] = m.tag;
                vm["digest"] = m.digest;
                vm["description"] = m.description;
                vm["size"] = m.size;
                qmlList.append(vm);
            }

            emit libraryFetched(qmlList);
            callback(true, models, "");
            reply->deleteLater();
        });
}

void OllamaModelStoreDialog::downloadLayer(const QString& modelName, const QString& digest, const QString& savePath)
{
    if (m_currentDownloadReply)
    {
        emit downloadFinished(false, "Download already in progress");
        return;
    }

    QString tag;
    QString cleanName = parseModelName(modelName, tag);

    QUrl url(QString("https://registry.ollama.com/v2/%1/blobs/%2").arg(cleanName, digest));
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    m_outputFile = new QFile(savePath);
    if (!m_outputFile->open(QIODevice::WriteOnly))
    {
        emit downloadFinished(false, "Could not open file for writing");
        delete m_outputFile;
        m_outputFile = nullptr;
        return;
    }

    m_currentDownloadReply = m_manager->get(request);

    connect(m_currentDownloadReply, &QNetworkReply::downloadProgress, this, &OllamaModelStoreDialog::downloadProgress);

    connect(m_currentDownloadReply, &QNetworkReply::readyRead, this,
        [this]()
        {
            if (m_outputFile && m_outputFile->isOpen())
            {
                m_outputFile->write(m_currentDownloadReply->readAll());
            }
        });

    connect(m_currentDownloadReply, &QNetworkReply::finished, this,
        [this, savePath]()
        {
            if (m_outputFile)
            {
                m_outputFile->close();
                delete m_outputFile;
                m_outputFile = nullptr;
            }

            if (m_currentDownloadReply->error() == QNetworkReply::NoError)
            {
                emit downloadFinished(true, savePath);
            }
            else
            {
                // Delete partial file on error
                QFile::remove(savePath);
                emit downloadFinished(false, m_currentDownloadReply->errorString());
            }

            m_currentDownloadReply->deleteLater();
            m_currentDownloadReply = nullptr;
        });
}

void OllamaModelStoreDialog::downloadModel(const QString& modelName, const QString& digest)
{
    // 1. Determine persistent storage path
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    if (!dir.exists("models"))
    {
        dir.mkpath("models");
    }

    // Sanitize filename
    QString safeName = modelName;
    safeName.replace('/', '_');
    safeName.replace(':', '_');
    if (!safeName.endsWith(".gguf"))
    {
        safeName += ".gguf";
    }

    QString fullPath = dir.filePath("models/" + safeName);
    qDebug() << "Downloading model to:" << fullPath;

    downloadLayer(modelName, digest, fullPath);
}

void OllamaModelStoreDialog::cancelDownload()
{
    if (m_currentDownloadReply)
    {
        m_currentDownloadReply->abort();
        // Cleanup happens in finished signal
    }
}
