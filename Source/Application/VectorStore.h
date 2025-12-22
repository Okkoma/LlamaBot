#pragma once

#include <QString>
#include <vector>

namespace RAG
{

struct SearchResult
{
    QString text;
    float score;
    QString source;
};

struct VectorEntry
{
    std::vector<float> embedding; // Normalized embedding
    QString text;
    QString source; // metadata
};

class VectorStore
{
public:
    VectorStore();

    bool load(const QString& path);
    bool save(const QString& path);
    void clear();

    void addEntry(const VectorEntry& entry);

    // Returns top K results sorted by similarity (descending)
    std::vector<SearchResult> search(const std::vector<float>& queryEmb, int topK);

    int count() const { return entries_.size(); }

private:
    std::vector<VectorEntry> entries_;

    // Helper: Cosine similarity between two normalized vectors is just their dot product
    static float cosineSimilarity(const std::vector<float>& a, const std::vector<float>& b);
};

}
