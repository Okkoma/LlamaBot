#include <QtTest>

#include "LLMService.h"

class LLMServiceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void test_construction();
};

void LLMServiceTest::initTestCase()
{
    // Initialisation globale éventuelle pour les tests
}

void LLMServiceTest::test_construction()
{
    LLMService service(nullptr);
    QVERIFY(true); // Si on arrive ici sans crash, la construction est OK
}

QTEST_MAIN(LLMServiceTest)
#include "tst_llmservice.moc"



