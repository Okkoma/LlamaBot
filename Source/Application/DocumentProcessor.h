#pragma once

#include <QString>
#include <vector>

namespace RAG
{

struct DocumentChunk
{
    QString content;
    QString sourceFile;
    int pageNumber; // -1 for text files
    int chunkIndex;
};

class DocumentProcessor
{
public:
    // Main entry point: processes a file and returns a list of chunks
    static std::vector<DocumentChunk> processFile(const QString& filePath, int chunkSize = 512, int overlap = 50);

private:
    // Extraction engines
    static QString extractTextFromPdf(const QString& path);
    static QString extractTextFromTxt(const QString& path);

    // Chunking logic
    static std::vector<QString> chunkText(const QString& text, int size, int overlap);
};

}
