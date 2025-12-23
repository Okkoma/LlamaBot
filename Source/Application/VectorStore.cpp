#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <algorithm>
#include <cmath>
#include <utility>

#include "VectorStore.h"

// Magic header for our file format
static const quint32 MAGIC = 0x52414731; // "RAG1"
static const quint32 VERSION = 1;

VectorStore::VectorStore() {}

void VectorStore::clear()
{
    entries_.clear();
}

bool VectorStore::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "VectorStore: Cannot open file for reading:" << path;
        return false;
    }

    QDataStream in(&file);

    quint32 magic;
    quint32 version;
    in >> magic >> version;

    if (magic != MAGIC)
    {
        qWarning() << "VectorStore: Invalid magic header in:" << path;
        return false;
    }

    if (version != VERSION)
    {
        qWarning() << "VectorStore: Unsupported version:" << version;
        return false;
    }

    quint32 count;
    in >> count;

    entries_.clear();
    entries_.reserve(count);

    for (quint32 i = 0; i < count; ++i)
    {
        VectorEntry entry;
        quint32 dim;
        in >> dim;
        entry.embedding.resize(dim);
        for (quint32 j = 0; j < dim; ++j)
        {
            in >> entry.embedding[j];
        }
        in >> entry.text >> entry.source;
        entries_.push_back(std::move(entry));
    }

    qDebug() << "VectorStore: Loaded" << entries_.size() << "entries from" << path;
    return true;
}

bool VectorStore::save(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "VectorStore: Cannot open file for writing:" << path;
        return false;
    }

    QDataStream out(&file);

    out << MAGIC << VERSION;
    out << (quint32)entries_.size();

    for (const auto& entry : entries_)
    {
        out << (quint32)entry.embedding.size();
        for (float val : entry.embedding)
        {
            out << val;
        }
        out << entry.text << entry.source;
    }

    return true;
}

void VectorStore::addEntry(const VectorEntry& entry)
{
    entries_.push_back(entry);
}

std::vector<SearchResult> VectorStore::search(const std::vector<float>& queryEmb, int topK)
{
    std::vector<SearchResult> results;
    if (entries_.empty() || queryEmb.empty())
        return results;

    // Use a pair of <score, index> to sort
    std::vector<std::pair<float, int>> scores;
    scores.reserve(entries_.size());

    for (size_t i = 0; i < entries_.size(); ++i)
    {
        // Assuming queryEmb is already normalized, and stored embeddings are normalized
        // Cosine Sim = Dot Product
        float score = cosineSimilarity(queryEmb, entries_[i].embedding);
        scores.push_back({ score, static_cast<int>(i) });
    }

    // Sort descending by score
    std::partial_sort(scores.begin(), scores.begin() + std::min((size_t)topK, scores.size()), scores.end(),
        [](const std::pair<float, int>& a, const std::pair<float, int>& b)
        {
            return a.first > b.first;
        });

    // Extract top K
    for (int i = 0; i < topK && i < (int)scores.size(); ++i)
    {
        const VectorEntry& entry = entries_[scores[i].second];
        results.push_back({ entry.text, scores[i].first, entry.source });
    }

    return results;
}

float VectorStore::cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b)
{
    if (a.size() != b.size())
        return 0.0f;

    float dot = 0.0f;
    for (size_t i = 0; i < a.size(); ++i)
    {
        dot += a[i] * b[i];
    }
    // Assumes vectors are normalized. If NOT, we would divide by (norm(a)*norm(b))
    return dot;
}

