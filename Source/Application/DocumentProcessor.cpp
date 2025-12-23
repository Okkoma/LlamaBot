#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRectF>
#include <QRegularExpression>
#include <QTextStream>
#include <poppler-qt6.h>

#include "DocumentProcessor.h"

std::vector<DocumentChunk> DocumentProcessor::processFile(const QString& filePath, int chunkSize, int overlap)
{
    std::vector<DocumentChunk> chunks;
    QFileInfo info(filePath);
    QString extension = info.suffix().toLower();
    QString fullText;
    int pageCount = 1;

    // 1. Extraction
    if (extension == "pdf")
    {
        // For PDFs, we might want to chunk page by page to keep page numbers accurate,
        // but for simplicity in this primitive phase, we'll process the whole text
        // OR process page by page. Let's process page by page to get better metadata.

        std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(filePath));
        if (!doc || doc->isLocked())
        {
            qWarning() << "DocumentProcessor: Failed to load PDF or it is locked:" << filePath;
            return chunks;
        }

        pageCount = doc->numPages();
        int globalChunkIndex = 0;

        for (int i = 0; i < pageCount; ++i)
        {
            std::unique_ptr<Poppler::Page> pdfPage(doc->page(i));
            if (!pdfPage)
                continue;

            QString pageText = pdfPage->text(QRectF()); // Extract text from whole page

            // Chunk this page
            std::vector<QString> textChunks = chunkText(pageText, chunkSize, overlap);
            for (const QString& text : textChunks)
            {
                chunks.push_back({ text, info.fileName(), i + 1, globalChunkIndex++ });
            }
        }
        return chunks;
    }
    else if (extension == "txt" || extension == "md")
    {
        fullText = extractTextFromTxt(filePath);
    }
    else
    {
        qWarning() << "DocumentProcessor: Unsupported file type:" << extension;
        return chunks;
    }

    // 2. Chunking for non-PDF (or if we had merged PDF text)
    if (!fullText.isEmpty())
    {
        std::vector<QString> textChunks = chunkText(fullText, chunkSize, overlap);
        int index = 0;
        for (const QString& text : textChunks)
        {
            chunks.push_back({ text, info.fileName(), -1, index++ });
        }
    }

    return chunks;
}

QString DocumentProcessor::extractTextFromPdf(const QString& path)
{
    // Note: We are doing page-by-page processing in processFile now.
    // This function is kept if we ever need raw full text extraction.
    std::unique_ptr<Poppler::Document> doc(Poppler::Document::load(path));
    if (!doc || doc->isLocked())
        return QString();

    QString fullText;
    for (int i = 0; i < doc->numPages(); ++i)
    {
        std::unique_ptr<Poppler::Page> pdfPage(doc->page(i));
        if (pdfPage)
        {
            fullText += pdfPage->text(QRectF());
            fullText += "\n";
        }
    }
    return fullText;
}

QString DocumentProcessor::extractTextFromTxt(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "DocumentProcessor: Cannot open text file:" << path;
        return QString();
    }
    QTextStream in(&file);
    return in.readAll();
}

std::vector<QString> DocumentProcessor::chunkText(const QString& text, int size, int overlap)
{
    std::vector<QString> result;
    if (text.isEmpty())
        return result;

    // Basic cleaning: collapse multiple spaces/newlines
    QString cleanText = text; // Copy
    cleanText.replace(QRegularExpression("\\s+"), " ");

    int totalLen = cleanText.length();
    int start = 0;

    while (start < totalLen)
    {
        int end = start + size;
        if (end >= totalLen)
        {
            end = totalLen;
        }
        else
        {
            // Try to find a space near the cut point to avoid splitting words
            int lookback = 0;
            // Look back up to 20 chars to find a space
            while (lookback < 20 && end - lookback > start && cleanText.at(end - lookback) != ' ')
            {
                lookback++;
            }
            if (cleanText.at(end - lookback) == ' ')
            {
                end -= lookback;
            }
        }

        QString chunk = cleanText.mid(start, end - start).trimmed();
        if (!chunk.isEmpty())
        {
            result.push_back(chunk);
        }

        // Start next chunk, considering overlap
        // If we reached the end, we break
        if (end == totalLen)
            break;

        start = end - overlap;

        // Ensure we advance at least by 1 to prevent Infinite Loop if overlap >= chunk size (bad config)
        if (start <= (end - size))
            start = end - size + 1;
        // More robust: ensure start is strictly greater than previous start
        // previous start was 'start' from start of loop. Let's start next one.
    }

    return result;
}
