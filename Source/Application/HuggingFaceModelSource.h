#pragma once

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>

#include "ModelSource.h"

/**
 * @brief Model source implementation for HuggingFace Hub
 * 
 * Uses the HuggingFace API (huggingface.co/api/models) to fetch
 * and download GGUF models.
 */
class HuggingFaceModelSource : public ModelSource
{
    Q_OBJECT

public:
    explicit HuggingFaceModelSource(QObject* parent = nullptr);
    ~HuggingFaceModelSource() override;

    QString sourceName() const override { return "HuggingFace"; }

    void fetchModels(SortOrder sort, SizeFilter sizeFilter, const QString& searchName,
        std::function<void(bool, const QVector<ModelManifest>&, const QString&)> callback) override;

    void fetchModelDetails(const QString& modelId,
        std::function<void(bool, const ModelDetails&, const QString&)> callback) override;

    void downloadFile(const QString& modelId, const QString& digest,
                      const QString& fileName, const QString& savePath) override;

private:
    QString sortOrderToApiParam(SortOrder sort);
};
