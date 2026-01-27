#pragma once

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>

/**
 * @brief Module de cryptage bout en bout pour les conversations
 * 
 * Utilise AES-256-GCM pour le chiffrement symétrique.
 * La clé est dérivée d'un mot de passe utilisateur via PBKDF2.
 * 
 * Format de sortie: base64(iv + ciphertext + tag)
 */
class Encryption
{
public:
    /**
     * @brief Chiffre un QJsonArray (liste de conversations)
     * @param data Données JSON à chiffrer
     * @param password Mot de passe pour dériver la clé de chiffrement
     * @return Données chiffrées en base64, ou QByteArray vide en cas d'erreur
     */
    static QByteArray encrypt(const QJsonArray& data, const QString& password);
    
    /**
     * @brief Déchiffre un QByteArray vers un QJsonArray
     * @param encryptedData Données chiffrées en base64
     * @param password Mot de passe utilisé pour le chiffrement
     * @return QJsonArray déchiffré, ou QJsonArray vide en cas d'erreur
     */
    static QJsonArray decrypt(const QByteArray& encryptedData, const QString& password);
    
    /**
     * @brief Vérifie si le mot de passe est correct pour des données chiffrées
     * @param encryptedData Données chiffrées en base64
     * @param password Mot de passe à vérifier
     * @return true si le mot de passe est correct, false sinon
     */
    static bool verifyPassword(const QByteArray& encryptedData, const QString& password);
    
    /**
     * @brief Génère une clé de chiffrement à partir d'un mot de passe
     * @param password Mot de passe utilisateur
     * @param salt Sel pour PBKDF2 (généré aléatoirement si vide)
     * @return Clé de 32 bytes (256 bits) pour AES-256
     */
    static QByteArray deriveKey(const QString& password, const QByteArray& salt = QByteArray());
    
    /**
     * @brief Génère un sel aléatoire pour PBKDF2
     * @return Sel de 16 bytes
     */
    static QByteArray generateSalt();

private:
    static constexpr int KEY_SIZE = 32;        // 256 bits pour AES-256
    static constexpr int IV_SIZE = 12;          // 96 bits pour GCM (recommandé)
    static constexpr int TAG_SIZE = 16;        // 128 bits pour le tag d'authentification
    static constexpr int SALT_SIZE = 16;        // 128 bits pour le sel PBKDF2
    static constexpr int PBKDF2_ITERATIONS = 100000; // Nombre d'itérations PBKDF2
};
