#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QtConcurrent/QtConcurrent>

#include "LLMService.h"
#include "RAGService.h"

RAGService::RAGService(LLMService* llmService, QObject* parent) :
    QObject(parent), llmService_(llmService), status_("Ready")
{
    // Try to load default collection on startup
    loadCollection();
}

RAGService::~RAGService() {}

void RAGService::ingestFile(const QString& filePath)
{
    status_ = "Ingesting " + QFileInfo(filePath).fileName() + "...";
    emit collectionStatusChanged();

    QFuture<void> f = QtConcurrent::run(
        [this, filePath]()
        {
            processFileInternal(filePath);
        });
}

void RAGService::ingestDirectory(const QString& dirPath)
{
    status_ = "Ingesting directory...";
    emit collectionStatusChanged();

    QFuture<void> f = QtConcurrent::run(
        [this, dirPath]()
        {
            QDirIterator it(
                dirPath, QStringList() << "*.pdf" << "*.txt" << "*.md", QDir::Files, QDirIterator::Subdirectories);
            int docs = 0;
            while (it.hasNext())
            {
                processFileInternal(it.next());
                docs++;
            }

            QMetaObject::invokeMethod(this,
                [this, docs]()
                {
                    status_ = QString("Ready (%1 docs ingested)").arg(docs);
                    emit collectionStatusChanged();
                    emit ingestionFinished(docs, vectorStore_.count());
                    saveCollection();
                });
        });
}

void RAGService::processFileInternal(const QString& filePath)
{
    if (!llmService_)
        return;

    // 1. Process Doc
    std::vector<DocumentChunk> chunks = DocumentProcessor::processFile(filePath);

    // 2. Compute Embeddings & Store
    for (const auto& chunk : chunks)
    {
        // Blocking call to get embedding (ensure your LLMService::getEmbedding is thread-safe or handles validation)
        std::vector<float> emb = llmService_->getEmbedding(chunk.content);

        if (!emb.empty())
        {
            VectorEntry entry;
            entry.embedding = emb;
            entry.text = chunk.content;
            entry.source = QString("%1 (Page %2)").arg(chunk.sourceFile).arg(chunk.pageNumber);

            // Normalize if not already
            // LLMService should return normalized embeddings, but let's be sure?
            // Assuming LLMService returns normalized for now.

            vectorStore_.addEntry(entry);
        }
    }
}

void RAGService::clearCollection()
{
    vectorStore_.clear();
    saveCollection();
    status_ = "Collection cleared";
    emit collectionStatusChanged();
}

bool RAGService::saveCollection()
{
    // Save to a default location in app data
    // For now, let's just save to bin/rag.db
    return vectorStore_.save("rag.db");
}

bool RAGService::loadCollection()
{
    bool ok = vectorStore_.load("rag.db");
    if (ok)
    {
        status_ = QString("Ready (%1 chunks loaded)").arg(vectorStore_.count());
        emit collectionStatusChanged();
    }
    return ok;
}

QString RAGService::retrieveContext(const QString& query, int topK)
{
    auto results = search(query, topK);
    QString context;
    for (const auto& res : results)
    {
        context += QString("[Source: %1]\n%2\n\n").arg(res.source, res.text);
    }
    return context;
}

std::vector<SearchResult> RAGService::search(const QString& query, int topK)
{
    if (!llmService_)
        return {};

    // Generate query embedding
    std::vector<float> queryEmb = llmService_->getEmbedding(query);
    if (queryEmb.empty())
        return {};

    return vectorStore_.search(queryEmb, topK);
}

QString RAGService::getCollectionStatus() const
{
    return status_;
}

