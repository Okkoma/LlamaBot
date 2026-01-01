#pragma once

#include "LLMServiceDefs.h"

class LLMServices;

/**
 * @struct ChatMessage
 * @brief Structure représentant un message dans un chat
 * 
 * Contient le rôle (utilisateur, assistant, système) et le contenu du message.
 */
struct ChatMessage
{
    QString role;      ///< Rôle du message (user, assistant, system)
    QString content;   ///< Contenu du message
};

/**
 * @struct ChatData
 * @brief Structure contenant les données de contexte du chat
 * 
 * Stocke les informations sur la taille du contexte et son utilisation.
 */
struct ChatData
{
    int n_ctx_{2048};        ///< Taille totale du contexte en tokens
    int n_ctx_used_{0};      ///< Nombre de tokens utilisés dans le contexte
};

/**
 * @class Chat
 * @brief Classe de base abstraite représentant un chat avec un modèle LLM
 * 
 * Cette classe définit l'interface commune pour tous les types de chats.
 * Elle gère l'historique des messages, le contexte, et fournit des méthodes
 * pour interagir avec les services LLM.
 */
class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(const QString& currentApi READ getCurrentApi WRITE setApi NOTIFY currentApiChanged)
    Q_PROPERTY(const QString& currentModel READ getCurrentModel WRITE setModel NOTIFY currentModelChanged)
    Q_PROPERTY(const QStringList& messages READ getMessages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList history READ historyList NOTIFY historyChanged)
    Q_PROPERTY(int tokensConsumed READ getNumContextSizeUsed NOTIFY messagesChanged)
    Q_PROPERTY(int tokensTotal READ getNumContextSize NOTIFY messagesChanged)

public:
    /**
     * @brief Constructeur de la classe Chat
     * @param llmservices Services LLM à utiliser pour ce chat
     * @param name Nom du chat (par défaut: "new_chat")
     * @param initialContext Contexte initial pour le chat
     * @param streamed Indique si le chat utilise le streaming (par défaut: true)
     * @param parent Objet parent Qt (optionnel)
     */
    Chat(LLMServices* llmservices, const QString& name = "new_chat", const QString& initialContext = "",
        bool streamed = true, QObject* parent = nullptr) :
        QObject(parent),
        llmservices_(llmservices),
        streamed_(streamed),
        processing_(false),
        name_(name),
        currentApi_("none"),
        currentModel_("none"),
        initialContext_(initialContext) {}
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~Chat() {}
    
    /**
     * @brief Définit l'API LLM à utiliser
     * @param api Nom de l'API à utiliser
     */
    virtual void setApi(const QString& api) = 0;
    
    /**
     * @brief Définit le modèle LLM à utiliser
     * @param model Nom du modèle à utiliser
     */
    virtual void setModel(const QString& model) = 0;
    
    /**
     * @brief Définit le nom du chat
     * @param name Nouveau nom pour le chat
     */
    void setName(const QString& name) { name_ = name; }
    
    /**
     * @brief Active ou désactive le streaming
     * @param enable true pour activer le streaming, false pour le désactiver
     */
    void setStreamed(bool enable) { streamed_ = enable; }
    
    /**
     * @brief Définit l'état de traitement du chat
     * @param processing true si le chat est en cours de traitement, false sinon
     * 
     * Émet les signaux appropriés en fonction de l'état.
     */
    void setProcessing(bool processing)
    {
        processing_ = processing;
        if (processing)
        {
            emit processingStarted(this);
        }
        else
        {
            if (streamed_)
                finalizeStream();
            emit processingFinished(this);
        }
    }

    /**
     * @brief Met à jour le contenu du chat
     * @param content Nouveau contenu à ajouter
     */
    virtual void updateContent(const QString& content) = 0;
    
    /**
     * @brief Met à jour le flux courant de l'IA
     * @param text Texte à ajouter au flux courant
     */
    virtual void updateCurrentAIStream(const QString& text) = 0;

    /**
     * @brief Met à jour les états de contexte
     * @param n_ctx Taille totale du contexte en tokens
     * @param n_ctx_used Nombre de tokens utilisés
     */
    void updateContextStates(int n_ctx, int n_ctx_used)
    { 
        data_.n_ctx_ = n_ctx; 
        data_.n_ctx_used_ = n_ctx_used;
    }

    /**
     * @brief Retourne le nom du chat
     * @return Nom du chat
     */
    const QString& getName() const { return name_; }
    
    /**
     * @brief Retourne si le streaming est activé
     * @return true si le streaming est activé, false sinon
     */
    bool getStreamed() const { return streamed_; }

    /**
     * @brief Retourne l'historique du chat sous forme de liste variante
     * @return Liste variante contenant l'historique
     */
    virtual QVariantList historyList() const = 0;
    
    /**
     * @brief Retourne l'API LLM courante
     * @return Nom de l'API courante
     */
    const QString& getCurrentApi() const { return currentApi_; }
    
    /**
     * @brief Retourne le modèle LLM courant
     * @return Nom du modèle courant
     */
    const QString& getCurrentModel() const { return currentModel_; }
    
    /**
     * @brief Retourne la liste des messages
     * @return Liste des messages sous forme de QStringList
     */
    const QStringList& getMessages() const { return messages_; }
    
    /**
     * @brief Retourne les messages bruts
     * @return Messages bruts sous forme de QString
     */
    const QString& getMessagesRaw() const { return messagesRaw_; }
    
    /**
     * @brief Retourne l'historique des messages
     * @return Référence vers la liste des messages structurés
     */
    QList<ChatMessage>& getHistory() { return history_; }
    
    /**
     * @brief Retourne les informations supplémentaires
     * @return Référence vers l'objet JSON contenant les informations
     */
    QJsonObject& getInfo() { return info_; }
    
    /**
     * @brief Retourne si le chat est en cours de traitement
     * @return true si le chat est en cours de traitement, false sinon
     */
    bool isProcessing() const { return processing_; }
    
    /**
     * @brief Retourne la taille totale du contexte
     * @return Taille totale du contexte en tokens
     */
    int getNumContextSize() const { return data_.n_ctx_; };
    
    /**
     * @brief Retourne le nombre de tokens utilisés dans le contexte
     * @return Nombre de tokens utilisés
     */
    int getNumContextSizeUsed() const { return data_.n_ctx_used_; };

    // Serialization
    /**
     * @brief Sérialise le chat en JSON
     * @return Objet JSON représentant le chat
     */
    virtual QJsonObject toJson() const = 0;
    
    /**
     * @brief Désérialise le chat depuis JSON
     * @param json Objet JSON contenant les données du chat
     */
    virtual void fromJson(const QJsonObject& json) = 0;

    // Export Helpers
    /**
     * @brief Retourne la conversation complète
     * @return Conversation complète sous forme de texte
     */
    Q_INVOKABLE virtual QString getFullConversation() const = 0;
    
    /**
     * @brief Retourne les prompts utilisateur
     * @return Tous les prompts utilisateur sous forme de texte
     */
    Q_INVOKABLE virtual QString getUserPrompts() const = 0;
    
    /**
     * @brief Retourne les réponses du bot
     * @return Toutes les réponses du bot sous forme de texte
     */
    Q_INVOKABLE virtual QString getBotResponses() const = 0;

signals:
    /**
     * @brief Signal émis lorsque l'API courante change
     */
    void currentApiChanged();
    
    /**
     * @brief Signal émis lorsque le modèle courant change
     */
    void currentModelChanged();
    
    /**
     * @brief Signal émis lorsque les messages changent
     */
    void messagesChanged();
    
    /**
     * @brief Signal émis lorsque le flux est mis à jour
     * @param text Texte du flux mis à jour
     */
    void streamUpdated(const QString& text); 
    
    /**
     * @brief Signal émis pour demander le nettoyage de l'entrée
     */
    void inputCleared();
    
    /**
     * @brief Signal émis lorsque le traitement d'un chat commence
     * @param chat Pointeur vers le chat en cours de traitement
     */
    void processingStarted(Chat* chat);
    
    /**
     * @brief Signal émis lorsque le traitement d'un chat se termine
     * @param Chat Pointeur vers le chat qui a terminé le traitement
     */
    void processingFinished(Chat* Chat);
    
    /**
     * @brief Signal émis lorsqu'un message est ajouté
     * @param role Rôle du message
     * @param content Contenu du message
     */
    void messageAdded(const QString& role, const QString& content);
    
    /**
     * @brief Signal émis lorsque le flux se termine
     */
    void streamFinishedSignal();
    
    /**
     * @brief Signal émis lorsque l'historique change
     */
    void historyChanged();

protected:
    /**
     * @brief Finalise le flux de streaming
     * 
     * Méthode appelée lorsque le streaming se termine pour nettoyer.
     */
    virtual void finalizeStream() = 0;

    // Data members
    ChatData data_;          ///< Données de contexte du chat

    bool streamed_;          ///< Indique si le streaming est activé
    bool processing_;        ///< Indique si le chat est en cours de traitement

    QString name_;           ///< Nom du chat
    QString currentApi_;     ///< API LLM courante
    QString currentModel_;   ///< Modèle LLM courant
    QString initialContext_; ///< Contexte initial du chat

    QStringList messages_;   ///< Liste des messages sous forme de texte
    QString messagesRaw_;    ///< Messages bruts

    QList<ChatMessage> history_;  ///< Historique des messages structurés
    QJsonObject info_;            ///< Informations supplémentaires en JSON

    LLMServices* llmservices_;   ///< Services LLM utilisés par ce chat
};
