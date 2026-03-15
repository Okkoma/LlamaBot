#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QSettings>
#include <QtConcurrent/QtConcurrent>

#include "DocumentProcessor.h"

#include "RAGService.h"

RAGService::RAGService(QObject* parent) :
    QObject(parent), status_("Ready")
{
    // Try to load default collection on startup
    loadCollection();

    enabled_ = QSettings().value("ragEnabled", false).toBool();
}

RAGService::~RAGService() {}

void RAGService::resetSettings()
{
    QSettings().setValue("ragEnabled",  false);
}

void RAGService::ingestFile(LLMService* service, const QString& filePath)
{
    status_ = "Ingesting " + QFileInfo(filePath).fileName() + "...";
    emit collectionStatusChanged();

    QFuture<void> f = QtConcurrent::run(
        [this, service, filePath]()
        {
            processFileInternal(service, filePath);
        });
}

void RAGService::ingestDirectory(LLMService* service, const QString& dirPath)
{
    status_ = "Ingesting directory...";
    emit collectionStatusChanged();

    service->setState(isEmbedding);

    QFuture<void> f = QtConcurrent::run(
        [this, service, dirPath]()
        {
            QDirIterator it(
                dirPath, QStringList() << "*.pdf" << "*.txt" << "*.md", QDir::Files, QDirIterator::Subdirectories);
            int docs = 0;
            while (it.hasNext())
            {
                processFileInternal(service, it.next());
                docs++;
            }

            QMetaObject::invokeMethod(this,
                [this, service, docs]()
                {
                    status_ = QString("Ready (%1 docs ingested)").arg(docs);
                    emit collectionStatusChanged();
                    emit ingestionFinished(docs, vectorStore_.count());
                    saveCollection();
                    service->setState(isWaiting);
                });
        });
}

void RAGService::processFileInternal(LLMService* service, const QString& filePath)
{
    // 1. Process Doc
    std::vector<DocumentChunk> chunks = DocumentProcessor::processFile(filePath);

    // 2. Compute Embeddings & Store
    for (const auto& chunk : chunks)
    {
        // Blocking call to get embedding (ensure your LLMServices::getEmbedding is thread-safe or handles validation)
        std::vector<float> emb = service->getEmbedding(chunk.content);

        if (!emb.empty())
        {
            VectorEntry entry;
            entry.embedding = emb;
            entry.text = chunk.content;
            entry.source = QString("%1 (Page %2)").arg(chunk.sourceFile).arg(chunk.pageNumber);

            // Normalize if not already
            // LLMServices should return normalized embeddings, but let's be sure?
            // Assuming LLMServices returns normalized for now.

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
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists())
        dir.mkpath(".");    
    return vectorStore_.save(dir.filePath("rag.db"));
}

bool RAGService::loadCollection()
{
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!dir.exists())
        dir.mkpath(".");    
    bool ok = vectorStore_.load(dir.filePath("rag.db"));
    if (ok)
    {
        status_ = QString("Ready (%1 chunks loaded)").arg(vectorStore_.count());
        emit collectionStatusChanged();
    }
    return ok;
}

QString RAGService::retrieveContext(LLMService* service, const QString& query)
{
    auto results = search(service, query);
    QString context;
    for (const auto& res : results)
    {
        context += QString("[Source: %1]\n%2\n\n").arg(res.source).arg(res.text);
    }
    return context;
}

std::vector<SearchResult> RAGService::search(LLMService* service, const QString& query)
{
    // Generate query embedding
    std::vector<float> queryEmb = service->getEmbedding(query);
    if (queryEmb.empty())
        return {};

    return vectorStore_.search(queryEmb, topK_);
}

QString RAGService::getCollectionStatus() const
{
    return status_;
}

