#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QJsonDocument>
#include <QDebug>

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/kdf.h>

#include "Encryption.h"

// Note: OpenSSL 3.0+ pour EVP_KDF (PBKDF2)
// Alternative si OpenSSL non disponible: utiliser Qt QCryptographicHash avec itérations manuelles

QByteArray Encryption::generateSalt()
{
    QByteArray salt(SALT_SIZE, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(salt.data()), SALT_SIZE) != 1)
    {
        qWarning() << "Encryption::generateSalt: RAND_bytes failed, using fallback";
        // Fallback: utiliser QRandomGenerator si OpenSSL RAND échoue
        QRandomGenerator* rng = QRandomGenerator::system();
        for (int i = 0; i < SALT_SIZE; ++i)
            salt[i] = static_cast<char>(rng->bounded(256));
    }
    return salt;
}

QByteArray Encryption::deriveKey(const QString& password, const QByteArray& salt)
{
    QByteArray key(KEY_SIZE, 0);
    QByteArray saltBytes = salt.isEmpty() ? generateSalt() : salt;
    
    // Utiliser OpenSSL EVP_KDF pour PBKDF2
    EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "PBKDF2", nullptr);
    if (!kdf)
    {
        qWarning() << "Encryption::deriveKey: EVP_KDF_fetch failed, using fallback";
        // Fallback: utiliser QCryptographicHash avec itérations manuelles (moins sécurisé)
        QByteArray data = password.toUtf8();
        for (int i = 0; i < PBKDF2_ITERATIONS; ++i)
        {
            data = QCryptographicHash::hash(data + saltBytes, QCryptographicHash::Sha256);
        }
        return data.left(KEY_SIZE);
    }
    
    EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    
    if (!kctx)
    {
        qWarning() << "Encryption::deriveKey: EVP_KDF_CTX_new failed";
        return QByteArray();
    }
    
    QByteArray passBytes = password.toUtf8();
    int iterations = PBKDF2_ITERATIONS;
    
    OSSL_PARAM params[5];
    params[0] = OSSL_PARAM_construct_utf8_string("digest", const_cast<char*>("SHA256"), 0);
    params[1] = OSSL_PARAM_construct_octet_string("pass", static_cast<void*>(passBytes.data()), passBytes.size());
    params[2] = OSSL_PARAM_construct_octet_string("salt", static_cast<void*>(saltBytes.data()), saltBytes.size());
    params[3] = OSSL_PARAM_construct_int("iter", &iterations);
    params[4] = OSSL_PARAM_construct_end();
    
    if (EVP_KDF_derive(kctx, reinterpret_cast<unsigned char*>(key.data()), KEY_SIZE, params) != 1)
    {
        qWarning() << "Encryption::deriveKey: EVP_KDF_derive failed";
        EVP_KDF_CTX_free(kctx);
        return QByteArray();
    }
    
    EVP_KDF_CTX_free(kctx);
    return key;
}

QByteArray Encryption::encrypt(const QJsonArray& data, const QString& password)
{
    if (password.isEmpty())
    {
        qWarning() << "Encryption::encrypt: password is empty";
        return QByteArray();
    }
    
    // Convertir JSON en bytes
    QJsonDocument doc(data);
    QByteArray plaintext = doc.toJson(QJsonDocument::Compact);
    
    // Générer IV et sel
    QByteArray salt = generateSalt();
    QByteArray iv(IV_SIZE, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(iv.data()), IV_SIZE) != 1)
    {
        qWarning() << "Encryption::encrypt: RAND_bytes failed for IV";
        return QByteArray();
    }
    
    // Dériver la clé
    QByteArray key = deriveKey(password, salt);
    if (key.isEmpty())
    {
        qWarning() << "Encryption::encrypt: key derivation failed";
        return QByteArray();
    }
    
    // Chiffrer avec AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        qWarning() << "Encryption::encrypt: EVP_CIPHER_CTX_new failed";
        return QByteArray();
    }
    
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
    {
        qWarning() << "Encryption::encrypt: EVP_EncryptInit_ex failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, 
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1)
    {
        qWarning() << "Encryption::encrypt: EVP_EncryptInit_ex (key/IV) failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    // Chiffrer le texte clair
    QByteArray ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()), 0);
    int len = 0;
    int ciphertextLen = 0;
    
    if (EVP_EncryptUpdate(ctx, 
                          reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                          reinterpret_cast<const unsigned char*>(plaintext.constData()),
                          plaintext.size()) != 1)
    {
        qWarning() << "Encryption::encrypt: EVP_EncryptUpdate failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    ciphertextLen = len;
    
    // Finaliser le chiffrement
    if (EVP_EncryptFinal_ex(ctx, 
                            reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len) != 1)
    {
        qWarning() << "Encryption::encrypt: EVP_EncryptFinal_ex failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    ciphertextLen += len;
    ciphertext.resize(ciphertextLen);
    
    // Obtenir le tag d'authentification
    QByteArray tag(TAG_SIZE, 0);
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag.data()) != 1)
    {
        qWarning() << "Encryption::encrypt: EVP_CIPHER_CTX_ctrl (GET_TAG) failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Format: salt (16) + iv (12) + ciphertext (variable) + tag (16)
    QByteArray result = salt + iv + ciphertext + tag;
    return result.toBase64();
}

QJsonArray Encryption::decrypt(const QByteArray& encryptedData, const QString& password)
{
    if (encryptedData.isEmpty() || password.isEmpty())
    {
        qWarning() << "Encryption::decrypt: empty input";
        return QJsonArray();
    }
    
    // Décoder base64
    QByteArray data = QByteArray::fromBase64(encryptedData);
    if (data.size() < SALT_SIZE + IV_SIZE + TAG_SIZE)
    {
        qWarning() << "Encryption::decrypt: data too short";
        return QJsonArray();
    }
    
    // Extraire salt, IV, ciphertext et tag
    QByteArray salt = data.left(SALT_SIZE);
    QByteArray iv = data.mid(SALT_SIZE, IV_SIZE);
    QByteArray tag = data.right(TAG_SIZE);
    QByteArray ciphertext = data.mid(SALT_SIZE + IV_SIZE, data.size() - SALT_SIZE - IV_SIZE - TAG_SIZE);
    
    // Dériver la clé
    QByteArray key = deriveKey(password, salt);
    if (key.isEmpty())
    {
        qWarning() << "Encryption::decrypt: key derivation failed";
        return QJsonArray();
    }
    
    // Déchiffrer avec AES-256-GCM
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        qWarning() << "Encryption::decrypt: EVP_CIPHER_CTX_new failed";
        return QJsonArray();
    }
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr) != 1)
    {
        qWarning() << "Encryption::decrypt: EVP_DecryptInit_ex failed";
        EVP_CIPHER_CTX_free(ctx);
        return QJsonArray();
    }
    
    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()),
                           reinterpret_cast<const unsigned char*>(iv.constData())) != 1)
    {
        qWarning() << "Encryption::decrypt: EVP_DecryptInit_ex (key/IV) failed";
        EVP_CIPHER_CTX_free(ctx);
        return QJsonArray();
    }
    
    // Déchiffrer le texte chiffré
    QByteArray plaintext(ciphertext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()), 0);
    int len = 0;
    int plaintextLen = 0;
    
    if (EVP_DecryptUpdate(ctx,
                          reinterpret_cast<unsigned char*>(plaintext.data()), &len,
                          reinterpret_cast<const unsigned char*>(ciphertext.constData()),
                          ciphertext.size()) != 1)
    {
        qWarning() << "Encryption::decrypt: EVP_DecryptUpdate failed";
        EVP_CIPHER_CTX_free(ctx);
        return QJsonArray();
    }
    plaintextLen = len;
    
    // Définir le tag avant de finaliser
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE, tag.data()) != 1)
    {
        qWarning() << "Encryption::decrypt: EVP_CIPHER_CTX_ctrl (SET_TAG) failed";
        EVP_CIPHER_CTX_free(ctx);
        return QJsonArray();
    }
    
    // Finaliser et vérifier l'authentification
    if (EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1)
    {
        qWarning() << "Encryption::decrypt: EVP_DecryptFinal_ex failed (wrong password or corrupted data)";
        EVP_CIPHER_CTX_free(ctx);
        return QJsonArray();
    }
    plaintextLen += len;
    plaintext.resize(plaintextLen);
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Parser JSON
    QJsonDocument doc = QJsonDocument::fromJson(plaintext);
    if (doc.isNull() || !doc.isArray())
    {
        qWarning() << "Encryption::decrypt: invalid JSON after decryption";
        return QJsonArray();
    }
    
    return doc.array();
}

bool Encryption::verifyPassword(const QByteArray& encryptedData, const QString& password)
{
    QJsonArray result = decrypt(encryptedData, password);
    return !result.isEmpty();
}
