#pragma once

#include "ModelSource.h"

#include "LLMServiceDefs.h"

/**
 * @brief Model source implementation for Ollama registry
 * 
 * Uses the Ollama API (ollama.com/api/tags) and 
 * (registry.ollama.com) to fetch and download models.
 */
class OllamaModelSource : public ModelSource
{
    Q_OBJECT

public:
    explicit OllamaModelSource(QObject* parent = nullptr);
    ~OllamaModelSource() override;

    QString sourceName() const override { return "Ollama"; }

    void fetchModels(SortOrder sort, SizeFilter sizeFilter, const QString& searchName,
        std::function<void(bool, const QVector<ModelManifest>&, const QString&)> callback) override;

    void fetchModelDetails(const QString& modelId,
        std::function<void(bool, const ModelDetails&, const QString&)> callback) override;

    void downloadFile(const QString& modelId, const QString& digest, 
                      const QString& fileName, const QString& savePath) override;

    static void getOllamaCloudModels(std::vector<LLMModel>& models);

private:
    QString parseModelName(const QString& input, QString& tag);

    static QVector<LLMModel> cloudModels_;
};
