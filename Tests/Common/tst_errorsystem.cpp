#include <QtTest>
#include <QDateTime>
#include <QThread>
#include <QTimer>

#include "../../Source/Common/ErrorSystem.h"

class ErrorSystemTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void test_singleton();
    void test_registerError();
    void test_logError_without_params();
    void test_logError_with_params();
    void test_logError_unregistered_code();
    void test_getErrors_empty_history();
    void test_getErrors_positive_index();
    void test_getErrors_negative_index();
    void test_getErrors_count_limit();
    void test_getErrors_count_zero();
    void test_getErrors_count_negative();
    void test_getErrors_index_out_of_bounds();
    void test_getErrors_formatting();
    void test_getErrors_multiple_params();
    void test_getErrors_order();
    void test_concurrent_access();
};

void ErrorSystemTest::initTestCase()
{
    qDebug() << "ErrorSystemTest::initTestCase()";
}

void ErrorSystemTest::cleanupTestCase()
{
    qDebug() << "ErrorSystemTest::cleanupTestCase()";    
}

void ErrorSystemTest::init()
{
    // Nettoyer l'historique avant chaque test
    ErrorSystem::instance().clearHistory();
}

void ErrorSystemTest::cleanup()
{
    // Pas de nettoyage nécessaire : fait avant chaque test
}

void ErrorSystemTest::test_singleton()
{
    ErrorSystem& instance1 = ErrorSystem::instance();
    ErrorSystem& instance2 = ErrorSystem::instance();
    
    // Vérifier que c'est la même instance
    QCOMPARE(&instance1, &instance2);
}

void ErrorSystemTest::test_registerError()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Erreur de test");
    int err2 = errorSystem.registerError("Autre erreur");
    QCOMPARE(errorSystem.getNumTypes(), 2);

    // Vérifier que les messages sont enregistrés en testant via logError
    errorSystem.logError(err1);
    QStringList errors = errorSystem.getErrors();
    QCOMPARE(errorSystem.size(), 1);
    QVERIFY(errors.first().contains("Erreur de test"));
}

void ErrorSystemTest::test_logError_without_params()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Message simple");
    errorSystem.logError(err1);

    QStringList errors = errorSystem.getErrors();
    QVERIFY(errors.first().contains("Message simple"));
    QVERIFY(errors.first().contains("[")); // Vérifier le format avec timestamp
}

void ErrorSystemTest::test_logError_with_params()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Erreur avec paramètre: %1");
    errorSystem.logError(err1, { "valeur1" });
    
    QStringList errors = errorSystem.getErrors(-1, 1);
    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.first().contains("Erreur avec paramètre: valeur1"));
}

void ErrorSystemTest::test_logError_unregistered_code()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int initialCount = errorSystem.getErrors().size();
    errorSystem.logError(9999); // Code non enregistré
    
    // L'erreur ne doit pas être ajoutée à l'historique
    QCOMPARE(errorSystem.getErrors().size(), initialCount);
}

void ErrorSystemTest::test_getErrors_empty_history()
{
    // On teste le cas où on demande des erreurs alors qu'il n'y en a pas
    ErrorSystem& errorSystem = ErrorSystem::instance();
    errorSystem.clearHistory();
    QCOMPARE(errorSystem.getErrors().size(), 0);
}

void ErrorSystemTest::test_getErrors_positive_index()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    // Enregistrer et logger plusieurs erreurs
    int err1 = errorSystem.registerError( "Erreur 1");
    int err2 = errorSystem.registerError( "Erreur 2");
    int err3 = errorSystem.registerError( "Erreur 3");
    
    errorSystem.logError(err1);
    QThread::msleep(10); // Petit délai pour avoir des timestamps différents
    errorSystem.logError(err2);
    QThread::msleep(10);
    errorSystem.logError(err3);
    
    // Tester index 0 (premier message)
    QStringList errors0 = errorSystem.getErrors(0, 1);
    QCOMPARE(errors0.size(), 1);
    QVERIFY(errors0.first().contains("Erreur 1"));
    
    // Tester index 1 (deuxième message)
    QStringList errors1 = errorSystem.getErrors(1, 1);
    QCOMPARE(errors1.size(), 1);
    QVERIFY(errors1.first().contains("Erreur 2"));
}

void ErrorSystemTest::test_getErrors_negative_index()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Dernière erreur");
    errorSystem.logError(err1);
    
    // Tester index -1 (dernier message)
    QStringList errors = errorSystem.getErrors(-1, 1);
    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.first().contains("Dernière erreur"));
    
    // Tester index -2 (avant-dernier message)
    int err2 = errorSystem.registerError("Avant-dernière erreur");
    errorSystem.logError(err2);
    
    QStringList errors2 = errorSystem.getErrors(-2, 1);
    QCOMPARE(errors2.size(), 1);
    QVERIFY(errors2.first().contains("Dernière erreur"));
}

void ErrorSystemTest::test_getErrors_count_limit()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Erreur A");
    int err2 = errorSystem.registerError("Erreur B");
    int err3 = errorSystem.registerError("Erreur C");
    
    errorSystem.logError(err1);
    errorSystem.logError(err2);
    errorSystem.logError(err3);
    
    // Demander seulement 2 messages
    QStringList errors = errorSystem.getErrors(0, 2);
    QCOMPARE(errors.size(), 2);
    QVERIFY(errors.first().contains("Erreur A"));
    QVERIFY(errors.last().contains("Erreur B"));
}

void ErrorSystemTest::test_getErrors_count_zero()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Erreur test");
    errorSystem.logError(err1);
    
    // Avec count = 0, devrait retourner tous les messages disponibles
    QStringList errors = errorSystem.getErrors(0, 0);
    QVERIFY(errors.size() > 0);
}

void ErrorSystemTest::test_getErrors_count_negative()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Erreur Y");
    int err2 = errorSystem.registerError("Erreur X");
    
    errorSystem.logError(err1);
    errorSystem.logError(err2);
    
    // Avec count = -1 (par défaut), devrait retourner tous les messages depuis l'index
    QStringList errors = errorSystem.getErrors(0, -1);
    QVERIFY(errors.size() >= 2);
}

void ErrorSystemTest::test_getErrors_index_out_of_bounds()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Seule erreur");
    errorSystem.logError(err1);
    
    // Index très grand devrait être clampé
    QStringList errors = errorSystem.getErrors(1000, 1);
    QVERIFY(errors.size() > 0); // Doit retourner au moins un message (clamp)
    
    // Index très négatif devrait être clampé
    QStringList errors2 = errorSystem.getErrors(-1000, 1);
    QVERIFY(errors2.size() > 0); // Doit retourner au moins un message (clamp)
}

void ErrorSystemTest::test_getErrors_formatting()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Message formaté");
    errorSystem.logError(err1);
    
    QStringList errors = errorSystem.getErrors(-1, 1);
    QCOMPARE(errors.size(), 1);
    
    QString error = errors.first();
    // Vérifier le format: [timestamp] message
    QVERIFY(error.startsWith("["));
    QVERIFY(error.contains("]"));
    QVERIFY(error.contains("Message formaté"));
}

void ErrorSystemTest::test_getErrors_multiple_params()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Erreur: %1, %2, %3");
    errorSystem.logError(err1, {"param1", "param2", "param3"});
    
    QStringList errors = errorSystem.getErrors(-1, 1);
    QCOMPARE(errors.size(), 1);
    QVERIFY(errors.first().contains("Erreur: param1, param2, param3"));
}

void ErrorSystemTest::test_getErrors_order()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Premier");
    int err2 = errorSystem.registerError("Deuxième");
    int err3 = errorSystem.registerError("Troisième");
    
    errorSystem.logError(err1);
    QThread::msleep(10);
    errorSystem.logError(err2);
    QThread::msleep(10);
    errorSystem.logError(err3);
    
    // Récupérer tous les messages depuis le début
    QStringList errors = errorSystem.getErrors(0, -1);
    QVERIFY(errors.size() >= 3);
    
    // Vérifier l'ordre: le premier doit contenir "Premier"
    QVERIFY(errors.first().contains("Premier"));
    // Le dernier doit contenir "Troisième"
    QVERIFY(errors.last().contains("Troisième"));
}

void ErrorSystemTest::test_concurrent_access()
{
    ErrorSystem& errorSystem = ErrorSystem::instance();
    
    int err1 = errorSystem.registerError("Concurrent test");
    
    // TODO : Simuler un accès concurrent simple
    // ici ce n'est pas un acces concurrent : créer plusieurs thread
    QList<QStringList> results;
    for (int i = 0; i < 10; ++i)
    {
        errorSystem.logError(err1);
        results.append(errorSystem.getErrors(-1, 1));
    }
    
    // Tous les appels doivent réussir sans crash
    QCOMPARE(results.size(), 10);
    for (const QStringList& result : results)    
        QVERIFY(!result.isEmpty());    
}

QTEST_MAIN(ErrorSystemTest)
#include "tst_errorsystem.moc"
