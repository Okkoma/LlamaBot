#pragma once

#include "LLMServiceDefs.h"

class OllamaManifest
{
public:
    OllamaManifest() = default;

    QString getModelFileName() const
    {
        for (const Layer& layer : layers_)
        {
            if (layer.mediaType_.contains("ollama.image.model"))
            {
                QString name = layer.digest_;
                return name.replace(':', '-');
            }
        }
        return QString("");
    }

    static OllamaManifest fromJson(const QJsonObject& obj)
    {
        OllamaManifest manifest;
        manifest.schemaVersion_ = obj["schemaVersion"].toInt();
        manifest.mediaType_ = obj["mediaType"].toString();
        if (obj.contains("config") && obj["config"].isObject())
        {
            QJsonObject configObj = obj["config"].toObject();
            manifest.config.mediaType_ = configObj["mediaType"].toString();
            manifest.config.digest_ = configObj["digest"].toString();
            manifest.config.size_ = configObj["size"].toVariant().toLongLong();
        }
        if (obj.contains("layers") && obj["layers"].isArray())
        {
            QJsonArray layersArray = obj["layers"].toArray();
            for (const QJsonValue& layerVal : layersArray)
            {
                if (layerVal.isObject())
                {
                    QJsonObject layerObj = layerVal.toObject();
                    Layer layer;
                    layer.mediaType_ = layerObj["mediaType"].toString();
                    layer.digest_ = layerObj["digest"].toString();
                    layer.size_ = layerObj["size"].toVariant().toLongLong();
                    manifest.layers_.append(layer);
                }
            }
        }
        return manifest;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["schemaVersion"] = schemaVersion_;
        obj["mediaType"] = mediaType_;
        QJsonObject configObj;
        configObj["mediaType"] = config.mediaType_;
        configObj["digest"] = config.digest_;
        configObj["size"] = QString::number(config.size_).toLongLong();
        obj["config"] = configObj;
        QJsonArray layersArray;
        for (const Layer& layer : layers_)
        {
            QJsonObject layerObj;
            layerObj["mediaType"] = layer.mediaType_;
            layerObj["digest"] = layer.digest_;
            layerObj["size"] = QString::number(layer.size_).toLongLong();
            layersArray.append(layerObj);
        }
        obj["layers"] = layersArray;
        return obj;
    }

    QString model_;
    QString num_params_;

    int schemaVersion_;
    QString mediaType_;
    struct Config
    {
        QString mediaType_;
        QString digest_;
        qint64 size_;
    } config;
    struct Layer
    {
        QString mediaType_;
        QString digest_;
        qint64 size_;
    };
    QList<Layer> layers_;
};

class OllamaService : public LLMAPIEntry
{
public:
    OllamaService(LLMService* service, const QString& name, const QString& url, const QString& ver, const QString& gen,
        const QString& apiKey, const QString& programPath, const QStringList& programArguments);
    ~OllamaService() override;

    bool start() override;
    bool stop() override;
    bool isReady() const override;

    void post(Chat* chat, const QString& content, bool streamed = true) override;

    bool handleMessageError(Chat* chat, const QString& message) override;

    QJsonObject toJson() const override;

    bool isUrlAccessible() const;
    bool isAPIAccessible() const;
    bool isProcessStarted() const;
    bool canStartProcess() const;

    std::vector<LLMModel> getAvailableModels() const override;

    static OllamaManifest getOllamaManifest(const QString& ollamaDir, const QString& model, const QString& num_params);
    static std::vector<OllamaManifest> getOllamaManifests(const QString& ollamaDir);
    static void getOllamaModels(const QString& ollamaDir, std::vector<LLMModel>& models);

    static const QString ollamaSystemDir;
    static const QString ollamaManifestBaseDir;
    static const QString ollamaBlobsBaseDir;

    QString url_;
    QString api_version_;
    QString api_generate_;
    QString apiKey_;

    QString programPath_;
    QStringList programArguments_;
    std::shared_ptr<QProcess> programProcess_;

private:
    void postInternal(Chat* chat, const QString& content, bool streamed);
};
