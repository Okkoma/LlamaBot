#include <QtTest>
#include <QTemporaryFile>
#include <QTemporaryDir>

#include "../../Source/Application/VectorStore.h"
#include "../../Source/Application/DocumentProcessor.h"

class RAGTest : public QObject
{
    Q_OBJECT

private slots:
    // VectorStore Tests
    void test_vector_store_add_and_search();
    void test_vector_store_persistence();
    void test_vector_store_empty_search();
    
    // DocumentProcessor Tests
    void test_document_processor_text_file();
    void test_document_processor_invalid_file();
};

void RAGTest::test_vector_store_add_and_search()
{
    VectorStore store;
    
    // Create some dummy entries with known embeddings
    VectorEntry e1;
    e1.text = "Apple";
    e1.embedding = {1.0f, 0.0f, 0.0f}; // Vector on X axis
    e1.source = "fruit.txt";
    store.addEntry(e1);
    
    VectorEntry e2;
    e2.text = "Banana";
    e2.embedding = {0.0f, 1.0f, 0.0f}; // Vector on Y axis
    e2.source = "fruit.txt";
    store.addEntry(e2);
    
    // Search for something close to X axis
    std::vector<float> query = {0.9f, 0.1f, 0.0f};
    auto results = store.search(query, 1);
    
    QCOMPARE(results.size(), 1UL);
    QCOMPARE(results[0].text, QString("Apple"));
    QVERIFY(results[0].score > 0.8f);
}

void RAGTest::test_vector_store_persistence()
{
    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    QVERIFY(tempFile.open());
    QString path = tempFile.fileName();
    tempFile.close();
    
    {
        VectorStore store;
        VectorEntry e;
        e.text = "Persistent Data";
        e.embedding = {0.5f, 0.5f, 0.5f};
        e.source = "test.txt";
        store.addEntry(e);
        QVERIFY(store.save(path));
    }
    
    {
        VectorStore store;
        QVERIFY(store.load(path));
        QCOMPARE(store.count(), 1);
        auto results = store.search({0.5f, 0.5f, 0.5f}, 1);
        QCOMPARE(results[0].text, QString("Persistent Data"));
    }
}

void RAGTest::test_vector_store_empty_search()
{
    VectorStore store;
    auto results = store.search({1.0f}, 5);
    QVERIFY(results.empty());
}

void RAGTest::test_document_processor_text_file()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QString filePath = dir.filePath("test.txt");
    
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    QString longContent = "This is a test document. It should be chunked properly by the DocumentProcessor class. "
                          "We are providing enough text to ensure that multiple chunks might be created if the chunk size is small enough.";
    file.write(longContent.toUtf8());
    file.close();
    
    // Test chunking with small size
    auto chunks = DocumentProcessor::processFile(filePath, 20, 5);
    QVERIFY(chunks.size() > 1);
    QCOMPARE(chunks[0].sourceFile, QFileInfo(filePath).fileName());
    QVERIFY(!chunks[0].content.isEmpty());
}

void RAGTest::test_document_processor_invalid_file()
{
    auto chunks = DocumentProcessor::processFile("/non/existent/path.txt");
    QVERIFY(chunks.empty());
}

QTEST_MAIN(RAGTest)
#include "tst_rag.moc"
