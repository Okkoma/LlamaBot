#pragma once

#include "DocumentProcessor.h"
#include "VectorStore.h"
#include <QFutureWatcher>
#include <QObject>

class LLMService;

namespace RAG
{

class RAGService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString collectionStatus READ getCollectionStatus NOTIFY collectionStatusChanged)

public:
    explicit RAGService(LLMService* llmService, QObject* parent = nullptr);
    ~RAGService();

    // Ingestion
    Q_INVOKABLE void ingestFile(const QString& filePath);
    Q_INVOKABLE void ingestDirectory(const QString& dirPath);
    Q_INVOKABLE void clearCollection();

    // Retrieval
    // Returns formatted context string for the prompt
    Q_INVOKABLE QString retrieveContext(const QString& query, int topK = 3);

    // Search returning raw results (useful for UI showing sources)
    std::vector<SearchResult> search(const QString& query, int topK = 3);

    // Persistence
    Q_INVOKABLE bool saveCollection();
    Q_INVOKABLE bool loadCollection();

    QString getCollectionStatus() const;

signals:
    void collectionStatusChanged();
    void ingestionFinished(int docsProcessed, int chunksAdded);
    void errorOccurred(const QString& error);

private:
    void processFileInternal(const QString& filePath);

    LLMService* llmService_;
    VectorStore vectorStore_;
    QString status_;

    // In-memory embedding cache or similar could go here
    // For now simple direct calls
};

}
