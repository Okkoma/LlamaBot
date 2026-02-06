#pragma once

#include <QObject>
#include <QSqlDatabase>
#include <QHttpServerRequest>
#include <QHttpServerResponse>
#include <QString>

/**
 * @brief Gestionnaire de connexion PostgreSQL
 * 
 * Gère la connexion à PostgreSQL et l'initialisation du schéma.
 */
class ChatDbManager : public QObject
{
    Q_OBJECT

public:
    explicit ChatDbManager(QObject* parent = nullptr);
    ~ChatDbManager() override;

    /**
     * @brief Établit la connexion à PostgreSQL
     */
    bool connect(const QString& host, int port, const QString& dbName,
                 const QString& user, const QString& password);

    /**
     * @brief Initialise le schéma de base de données (création des tables si nécessaire)
     */
    bool initializeSchema();

    /**
     * @brief Retourne la connexion SQL active
     */
    QSqlDatabase& database() { return db_; }

    /**
     * @brief Vérifie si la connexion est valide
     */
    bool isValid() const { return db_.isValid() && db_.isOpen(); }

    /**
     * @brief Liste toutes les conversations d'un utilisateur
     * GET /conversations?user_id=xxx
     */
     QHttpServerResponse listConversations(const QHttpServerRequest& request);

     /**
      * @brief Récupère une conversation spécifique
      * GET /conversations/{id}
      */
     QHttpServerResponse getConversation(const QString& id, const QHttpServerRequest& request);
  
     /**
      * @brief Crée ou met à jour une conversation
      * POST /conversations
      * Body: { "id": "uuid", "user_id": "xxx", "encrypted_data": "base64..." }
     */
     QHttpServerResponse saveConversation(const QHttpServerRequest& request);
  
     /**
      * @brief Met à jour une conversation existante
      * PUT /conversations/{id}
      */
     QHttpServerResponse updateConversation(const QString& id, const QHttpServerRequest& request);
  
     /**
      * @brief Supprime une conversation (soft delete)
      * DELETE /conversations/{id}
      */
     QHttpServerResponse deleteConversation(const QString& id, const QHttpServerRequest& request);
  
     /**
       * @brief Synchronisation batch de plusieurs conversations
       * POST /conversations/sync
       * Body: { "user_id": "xxx", "conversations": [...] }
      */
     QHttpServerResponse syncConversations(const QHttpServerRequest& request);
         
private:
    /**
     * @brief Crée les tables si elles n'existent pas
     */     
    bool createTables();

    /**
     * @brief Extrait user_id depuis les query params ou headers
     */
    QString extractUserId(const QHttpServerRequest& request) const;

    /**
     * @brief Valide le format UUID
     */
    bool isValidUuid(const QString& id) const;

    QSqlDatabase db_;
    QString connectionName_;
};
