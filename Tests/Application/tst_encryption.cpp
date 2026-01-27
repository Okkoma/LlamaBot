#include <QtTest>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>

#include "../../Source/Application/Encryption.h"

/**
 * @brief Tests unitaires pour Encryption
 * 
 * Ces tests vérifient:
 * - La génération de sel aléatoire
 * - La dérivation de clé via PBKDF2
 * - Le chiffrement et déchiffrement de données JSON avec AES-256-GCM
 * - La vérification de mot de passe
 * - La gestion des erreurs (mot de passe vide, données corrompues, etc.)
 * - Les cas limites (données vides, caractères spéciaux, grandes données, etc.)
 */
class EncryptionTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Tests de génération de sel
    void test_generateSalt_returns_correct_size();
    void test_generateSalt_returns_different_values();
    void test_generateSalt_not_empty();

    // Tests de dérivation de clé
    void test_deriveKey_returns_correct_size();
    void test_deriveKey_same_password_same_salt_same_key();
    void test_deriveKey_different_password_different_key();
    void test_deriveKey_different_salt_different_key();
    void test_deriveKey_empty_password();
    void test_deriveKey_with_special_characters();

    // Tests de chiffrement/déchiffrement de base
    void test_encrypt_decrypt_simple_json();
    void test_encrypt_decrypt_empty_json_array();
    void test_encrypt_decrypt_complex_json();
    void test_encrypt_decrypt_with_special_characters();
    void test_encrypt_decrypt_with_unicode();
    void test_encrypt_decrypt_large_json();

    // Tests de gestion des erreurs
    void test_encrypt_with_empty_password();
    void test_decrypt_with_empty_password();
    void test_decrypt_with_empty_data();
    void test_decrypt_with_wrong_password();
    void test_decrypt_with_corrupted_data();
    void test_decrypt_with_invalid_base64();
    void test_decrypt_with_truncated_data();
    void test_decrypt_with_modified_ciphertext();
    void test_decrypt_with_modified_tag();

    // Tests de vérification de mot de passe
    void test_verifyPassword_correct_password();
    void test_verifyPassword_wrong_password();
    void test_verifyPassword_empty_password();
    void test_verifyPassword_empty_data();

    // Tests de cas limites
    void test_encrypt_decrypt_multiple_messages();
    void test_encrypt_decrypt_nested_json();
    void test_encrypt_decrypt_with_long_password();
    void test_encrypt_decrypt_preserves_json_structure();
    void test_encrypt_returns_base64();
    void test_different_encryptions_same_data_different_output();

private:
    QJsonArray createSimpleJsonArray();
    QJsonArray createComplexJsonArray();
    QJsonArray createLargeJsonArray();
};

void EncryptionTest::initTestCase()
{
    qDebug() << "EncryptionTest::initTestCase()";
}

void EncryptionTest::cleanupTestCase()
{
    qDebug() << "EncryptionTest::cleanupTestCase()";
}

void EncryptionTest::init()
{
    // Rien à faire avant chaque test
}

void EncryptionTest::cleanup()
{
    // Rien à faire après chaque test
}

QJsonArray EncryptionTest::createSimpleJsonArray()
{
    QJsonArray array;
    QJsonObject obj;
    obj["id"] = "test-id-1";
    obj["name"] = "Test Chat";
    obj["content"] = "Hello World";
    array.append(obj);
    return array;
}

QJsonArray EncryptionTest::createComplexJsonArray()
{
    QJsonArray array;
    
    for (int i = 0; i < 3; ++i)
    {
        QJsonObject chat;
        chat["id"] = QString("chat-%1").arg(i);
        chat["name"] = QString("Chat %1").arg(i);
        chat["api"] = "TestAPI";
        chat["model"] = "test-model";
        
        QJsonArray messages;
        for (int j = 0; j < 5; ++j)
        {
            QJsonObject msg;
            msg["role"] = (j % 2 == 0) ? "user" : "assistant";
            msg["content"] = QString("Message %1-%2").arg(i).arg(j);
            messages.append(msg);
        }
        
        chat["messages"] = messages;
        array.append(chat);
    }
    
    return array;
}

QJsonArray EncryptionTest::createLargeJsonArray()
{
    QJsonArray array;
    
    for (int i = 0; i < 100; ++i)
    {
        QJsonObject obj;
        obj["id"] = QString("id-%1").arg(i);
        obj["content"] = QString("Content ").repeated(1000); // ~8KB par objet
        array.append(obj);
    }
    
    return array;
}

// Tests de génération de sel

void EncryptionTest::test_generateSalt_returns_correct_size()
{
    qDebug() << "EncryptionTest::test_generateSalt_returns_correct_size()";
    
    QByteArray salt = Encryption::generateSalt();
    QCOMPARE(salt.size(), 16); // SALT_SIZE = 16
}

void EncryptionTest::test_generateSalt_returns_different_values()
{
    qDebug() << "EncryptionTest::test_generateSalt_returns_different_values()";
    
    QByteArray salt1 = Encryption::generateSalt();
    QByteArray salt2 = Encryption::generateSalt();
    
    // Les deux sels devraient être différents (probabilité extrêmement faible qu'ils soient identiques)
    QVERIFY(salt1 != salt2);
}

void EncryptionTest::test_generateSalt_not_empty()
{
    qDebug() << "EncryptionTest::test_generateSalt_not_empty()";
    
    QByteArray salt = Encryption::generateSalt();
    QVERIFY(!salt.isEmpty());
}

// Tests de dérivation de clé

void EncryptionTest::test_deriveKey_returns_correct_size()
{
    qDebug() << "EncryptionTest::test_deriveKey_returns_correct_size()";
    
    QByteArray salt = Encryption::generateSalt();
    QByteArray key = Encryption::deriveKey("password123", salt);
    
    QCOMPARE(key.size(), 32); // KEY_SIZE = 32 (256 bits)
}

void EncryptionTest::test_deriveKey_same_password_same_salt_same_key()
{
    qDebug() << "EncryptionTest::test_deriveKey_same_password_same_salt_same_key()";
    
    QByteArray salt = Encryption::generateSalt();
    QByteArray key1 = Encryption::deriveKey("password123", salt);
    QByteArray key2 = Encryption::deriveKey("password123", salt);
    
    QCOMPARE(key1, key2);
}

void EncryptionTest::test_deriveKey_different_password_different_key()
{
    qDebug() << "EncryptionTest::test_deriveKey_different_password_different_key()";
    
    QByteArray salt = Encryption::generateSalt();
    QByteArray key1 = Encryption::deriveKey("password123", salt);
    QByteArray key2 = Encryption::deriveKey("password456", salt);
    
    QVERIFY(key1 != key2);
}

void EncryptionTest::test_deriveKey_different_salt_different_key()
{
    qDebug() << "EncryptionTest::test_deriveKey_different_salt_different_key()";
    
    QByteArray salt1 = Encryption::generateSalt();
    QByteArray salt2 = Encryption::generateSalt();
    QByteArray key1 = Encryption::deriveKey("password123", salt1);
    QByteArray key2 = Encryption::deriveKey("password123", salt2);
    
    QVERIFY(key1 != key2);
}

void EncryptionTest::test_deriveKey_empty_password()
{
    qDebug() << "EncryptionTest::test_deriveKey_empty_password()";
    
    QByteArray salt = Encryption::generateSalt();
    QByteArray key = Encryption::deriveKey("", salt);
    
    // Même avec un mot de passe vide, une clé devrait être générée
    QCOMPARE(key.size(), 32);
}

void EncryptionTest::test_deriveKey_with_special_characters()
{
    qDebug() << "EncryptionTest::test_deriveKey_with_special_characters()";
    
    QByteArray salt = Encryption::generateSalt();
    QString specialPassword = "p@ssw0rd!#$%^&*()éàèûç🔒";
    QByteArray key = Encryption::deriveKey(specialPassword, salt);
    
    QCOMPARE(key.size(), 32);
    QVERIFY(!key.isEmpty());
}

// Tests de chiffrement/déchiffrement de base

void EncryptionTest::test_encrypt_decrypt_simple_json()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_simple_json()";
    
    QJsonArray original = createSimpleJsonArray();
    QString password = "testPassword123";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QVERIFY(!decrypted.isEmpty());
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_empty_json_array()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_empty_json_array()";
    
    QJsonArray original;
    QString password = "testPassword123";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_complex_json()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_complex_json()";
    
    QJsonArray original = createComplexJsonArray();
    QString password = "complexPassword!@#";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_with_special_characters()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_with_special_characters()";
    
    QJsonArray original;
    QJsonObject obj;
    obj["content"] = "Texte avec caractères spéciaux: éàèûç@#$%^&*()";
    obj["emoji"] = "🤖 🎉 ✨ 🔒";
    original.append(obj);
    
    QString password = "p@ssw0rd!éàèûç";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_with_unicode()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_with_unicode()";
    
    QJsonArray original;
    QJsonObject obj;
    obj["chinese"] = "你好世界";
    obj["arabic"] = "مرحبا بالعالم";
    obj["russian"] = "Привет мир";
    obj["emoji"] = "😀😁😂🤣😃😄";
    original.append(obj);
    
    QString password = "unicodePassword123";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_large_json()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_large_json()";
    
    QJsonArray original = createLargeJsonArray();
    QString password = "largeDataPassword";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted.size(), original.size());
    QCOMPARE(decrypted, original);
}

// Tests de gestion des erreurs

void EncryptionTest::test_encrypt_with_empty_password()
{
    qDebug() << "EncryptionTest::test_encrypt_with_empty_password()";
    
    QJsonArray data = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(data, "");
    
    QVERIFY(encrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_empty_password()
{
    qDebug() << "EncryptionTest::test_decrypt_with_empty_password()";
    
    QJsonArray original = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(original, "password123");
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, "");
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_empty_data()
{
    qDebug() << "EncryptionTest::test_decrypt_with_empty_data()";
    
    QByteArray emptyData;
    QJsonArray decrypted = Encryption::decrypt(emptyData, "password123");
    
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_wrong_password()
{
    qDebug() << "EncryptionTest::test_decrypt_with_wrong_password()";
    
    QJsonArray original = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(original, "correctPassword");
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, "wrongPassword");
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_corrupted_data()
{
    qDebug() << "EncryptionTest::test_decrypt_with_corrupted_data()";
    
    QJsonArray original = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(original, "password123");
    
    // Corrompre les données
    QByteArray corrupted = encrypted;
    if (corrupted.size() > 10)
    {
        corrupted[10] = ~corrupted[10]; // Inverser un byte
    }
    
    QJsonArray decrypted = Encryption::decrypt(corrupted, "password123");
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_invalid_base64()
{
    qDebug() << "EncryptionTest::test_decrypt_with_invalid_base64()";
    
    QByteArray invalidBase64 = "This is not valid base64!@#$%";
    QJsonArray decrypted = Encryption::decrypt(invalidBase64, "password123");
    
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_truncated_data()
{
    qDebug() << "EncryptionTest::test_decrypt_with_truncated_data()";
    
    QJsonArray original = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(original, "password123");
    
    // Tronquer les données (enlever les derniers bytes)
    QByteArray truncated = encrypted.left(encrypted.size() / 2);
    
    QJsonArray decrypted = Encryption::decrypt(truncated, "password123");
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_modified_ciphertext()
{
    qDebug() << "EncryptionTest::test_decrypt_with_modified_ciphertext()";
    
    QJsonArray original = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(original, "password123");
    
    // Décoder base64, modifier le ciphertext, et ré-encoder
    QByteArray decoded = QByteArray::fromBase64(encrypted);
    if (decoded.size() > 50)
    {
        decoded[40] = ~decoded[40]; // Modifier un byte dans le ciphertext
    }
    QByteArray modified = decoded.toBase64();
    
    QJsonArray decrypted = Encryption::decrypt(modified, "password123");
    // GCM devrait détecter la modification et échouer
    QVERIFY(decrypted.isEmpty());
}

void EncryptionTest::test_decrypt_with_modified_tag()
{
    qDebug() << "EncryptionTest::test_decrypt_with_modified_tag()";
    
    QJsonArray original = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(original, "password123");
    
    // Décoder base64, modifier le tag (derniers 16 bytes), et ré-encoder
    QByteArray decoded = QByteArray::fromBase64(encrypted);
    if (decoded.size() > 16)
    {
        decoded[decoded.size() - 1] = ~decoded[decoded.size() - 1]; // Modifier le tag
    }
    QByteArray modified = decoded.toBase64();
    
    QJsonArray decrypted = Encryption::decrypt(modified, "password123");
    // GCM devrait détecter la modification du tag et échouer
    QVERIFY(decrypted.isEmpty());
}

// Tests de vérification de mot de passe

void EncryptionTest::test_verifyPassword_correct_password()
{
    qDebug() << "EncryptionTest::test_verifyPassword_correct_password()";
    
    QJsonArray data = createSimpleJsonArray();
    QString password = "correctPassword123";
    QByteArray encrypted = Encryption::encrypt(data, password);
    
    bool result = Encryption::verifyPassword(encrypted, password);
    QVERIFY(result);
}

void EncryptionTest::test_verifyPassword_wrong_password()
{
    qDebug() << "EncryptionTest::test_verifyPassword_wrong_password()";
    
    QJsonArray data = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(data, "correctPassword");
    
    bool result = Encryption::verifyPassword(encrypted, "wrongPassword");
    QVERIFY(!result);
}

void EncryptionTest::test_verifyPassword_empty_password()
{
    qDebug() << "EncryptionTest::test_verifyPassword_empty_password()";
    
    QJsonArray data = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(data, "password123");
    
    bool result = Encryption::verifyPassword(encrypted, "");
    QVERIFY(!result);
}

void EncryptionTest::test_verifyPassword_empty_data()
{
    qDebug() << "EncryptionTest::test_verifyPassword_empty_data()";
    
    QByteArray emptyData;
    bool result = Encryption::verifyPassword(emptyData, "password123");
    
    QVERIFY(!result);
}

// Tests de cas limites

void EncryptionTest::test_encrypt_decrypt_multiple_messages()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_multiple_messages()";
    
    QJsonArray original;
    for (int i = 0; i < 50; ++i)
    {
        QJsonObject obj;
        obj["id"] = QString("msg-%1").arg(i);
        obj["content"] = QString("Message content %1").arg(i);
        original.append(obj);
    }
    
    QString password = "multiMessagePassword";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted.size(), 50);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_nested_json()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_nested_json()";
    
    QJsonArray original;
    QJsonObject obj;
    obj["id"] = "nested-1";
    
    QJsonObject nested1;
    nested1["key1"] = "value1";
    nested1["key2"] = 42;
    
    QJsonArray nestedArray;
    nestedArray.append("item1");
    nestedArray.append("item2");
    nestedArray.append("item3");
    
    nested1["array"] = nestedArray;
    obj["nested"] = nested1;
    
    original.append(obj);
    
    QString password = "nestedPassword";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_with_long_password()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_with_long_password()";
    
    QJsonArray original = createSimpleJsonArray();
    QString longPassword = QString("VeryLongPassword").repeated(100); // ~1600 caractères
    
    QByteArray encrypted = Encryption::encrypt(original, longPassword);
    QVERIFY(!encrypted.isEmpty());
    
    QJsonArray decrypted = Encryption::decrypt(encrypted, longPassword);
    QCOMPARE(decrypted, original);
}

void EncryptionTest::test_encrypt_decrypt_preserves_json_structure()
{
    qDebug() << "EncryptionTest::test_encrypt_decrypt_preserves_json_structure()";
    
    QJsonArray original;
    QJsonObject obj;
    obj["string"] = "text";
    obj["number"] = 42;
    obj["float"] = 3.14159;
    obj["bool"] = true;
    obj["null"] = QJsonValue::Null;
    
    QJsonArray subArray;
    subArray.append(1);
    subArray.append(2);
    subArray.append(3);
    obj["array"] = subArray;
    
    original.append(obj);
    
    QString password = "structurePassword";
    
    QByteArray encrypted = Encryption::encrypt(original, password);
    QJsonArray decrypted = Encryption::decrypt(encrypted, password);
    
    QCOMPARE(decrypted, original);
    
    // Vérifier les types spécifiques
    QJsonObject decryptedObj = decrypted[0].toObject();
    QVERIFY(decryptedObj["string"].isString());
    QVERIFY(decryptedObj["number"].isDouble());
    QVERIFY(decryptedObj["float"].isDouble());
    QVERIFY(decryptedObj["bool"].isBool());
    QVERIFY(decryptedObj["null"].isNull());
    QVERIFY(decryptedObj["array"].isArray());
}

void EncryptionTest::test_encrypt_returns_base64()
{
    qDebug() << "EncryptionTest::test_encrypt_returns_base64()";
    
    QJsonArray data = createSimpleJsonArray();
    QByteArray encrypted = Encryption::encrypt(data, "password123");
    
    // Vérifier que c'est du base64 valide
    QByteArray decoded = QByteArray::fromBase64(encrypted);
    QByteArray reencoded = decoded.toBase64();
    
    QCOMPARE(encrypted, reencoded);
}

void EncryptionTest::test_different_encryptions_same_data_different_output()
{
    qDebug() << "EncryptionTest::test_different_encryptions_same_data_different_output()";
    
    QJsonArray data = createSimpleJsonArray();
    QString password = "password123";
    
    QByteArray encrypted1 = Encryption::encrypt(data, password);
    QByteArray encrypted2 = Encryption::encrypt(data, password);
    
    // Les deux chiffrements devraient être différents (IV différent)
    QVERIFY(encrypted1 != encrypted2);
    
    // Mais les deux devraient se déchiffrer correctement
    QJsonArray decrypted1 = Encryption::decrypt(encrypted1, password);
    QJsonArray decrypted2 = Encryption::decrypt(encrypted2, password);
    
    QCOMPARE(decrypted1, data);
    QCOMPARE(decrypted2, data);
}

QTEST_MAIN(EncryptionTest)
#include "tst_encryption.moc"
