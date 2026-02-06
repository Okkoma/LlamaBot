#pragma once

#include <QNetworkAccessManager>
#include <QUrl>
#include <QString>

#include "ChatStorage.h"

class QNetworkReply;

/**
 * @brief Stockage/synchronisation distante (PostgreSQL via API serveur)
 *
 * Les données sont chiffrées bout en bout avant l'envoi au serveur.
 * Le serveur stocke uniquement les données chiffrées (il ne peut jamais les déchiffrer).
 */
class ChatStorageDistant final : public ChatStorage
{
    Q_OBJECT
public:
    /**
     * @brief Constructeur
     * @param llmservices LLMServices     
     * @param serverUrl URL de base du serveur (ex: "http://localhost:8080")
     * @param userId Identifiant utilisateur pour l'authentification
     * @param encryptionPassword Mot de passe pour le chiffrement bout en bout
     */
    explicit ChatStorageDistant(LLMServices* llmservices, const QString& serverUrl = QString(),
                             const QString& userId = QString(),
                             const QString& encryptionPassword = QString());
    
    ~ChatStorageDistant() override = default;

    bool load(QList<Chat*>& chats) override;
    bool save(const QList<Chat*>& chats) override;

    /**
     * @brief Définit l'URL du serveur
     */
    void setServerUrl(const QString& url) { serverUrl_ = url; }

    /**
     * @brief Définit l'identifiant utilisateur
     */
    void setUserId(const QString& userId) { userId_ = userId; }

    /**
     * @brief Définit le mot de passe de chiffrement
     */
    void setEncryptionPassword(const QString& password) { encryptionPassword_ = password; }

private slots:
    void onLoadFinished();
    void onSaveFinished();
    void onSyncFinished();

private:
    QNetworkAccessManager* networkManager_;
    QString serverUrl_;
    QString userId_;
    QString encryptionPassword_;
    
    QNetworkReply* pendingLoadReply_;
    QNetworkReply* pendingSaveReply_;
    QNetworkReply* pendingSyncReply_;
    
    QJsonArray pendingLoadResult_;
    bool pendingLoadSuccess_;
    bool pendingSaveSuccess_;
    bool pendingSyncSuccess_;

    /**
     * @brief Construit l'URL complète pour une route
     */
    QUrl buildUrl(const QString& path) const;

    /**
     * @brief Construit les headers HTTP pour une requête
     */
    QNetworkRequest buildRequest(const QUrl& url) const;

    /**
     * @brief Charge les conversations depuis le serveur distant
     * @return QJsonArray des conversations déchiffrées, ou std::nullopt en cas d'erreur
     */
     std::optional<QJsonArray> loadJson();

     /**
      * @brief Sauvegarde les conversations sur le serveur distant
      * @param chats Données JSON à sauvegarder (seront chiffrées avant envoi)
      * @return true si la sauvegarde a réussi, false sinon
      */
      
     bool saveJson(const QJsonArray& chats);
    /**
     * @brief Synchronise plusieurs conversations en batch
     * @param conversations Liste des conversations à synchroniser
     * @return true si la synchronisation a réussi, false sinon
     */
    bool syncConversations(const QJsonArray& conversations);
};
