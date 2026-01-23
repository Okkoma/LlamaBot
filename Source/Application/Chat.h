#pragma once

#include "LLMServiceDefs.h"

#include <QUuid>

class LLMServices;

/**
 * @struct ChatMessage
 * @brief Structure représentant un message dans un chat
 * 
 * Contient le rôle (utilisateur, assistant, système) et le contenu du message.
 */
struct ChatMessage
{
    ChatMessage(const QString& role, const QString& content, const QVariantList& assets = QVariantList()) :
        role_(role),
        content_(content),
        assets_(assets) {}
    QString role_;        ///< Rôle du message (user, assistant, system)
    QString content_;     ///< Contenu du message
    QVariantList assets_; ///< Contenu des assets
};

/**
 * @struct ChatData
 * @brief Structure contenant les données de contexte du chat
 * 
 * Stocke les informations sur la taille du contexte et son utilisation.
 */
struct ChatData
{
    int n_ctx_{LLM_DEFAULT_CONTEXT_SIZE}; ///< Taille totale du contexte en tokens
    int n_ctx_used_{0};      ///< Nombre de tokens utilisés dans le contexte
    std::vector<int> context_tokens_;
    
    virtual void reset() { };
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
    Q_PROPERTY(int contextSizeUsed READ getContextSizeUsed NOTIFY contextSizeUsedChanged)
    Q_PROPERTY(int contextSize READ getContextSize WRITE setContextSize NOTIFY contextSizeChanged)

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
        id_(QUuid::createUuid().toString(QUuid::WithoutBraces)),
        name_(name),
        currentApi_("none"),
        currentModel_("none"),
        initialContext_(initialContext) {}
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~Chat() {}

    /**
     * @brief Retourne l'identifiant stable du chat (persisté)
     */
    const QString& getId() const { return id_; }

    /**
     * @brief Définit l'identifiant stable du chat (utile lors d'un chargement depuis stockage)
     */
    void setId(const QString& id)
    {
        if (!id.isEmpty())
            id_ = id;
    }
    
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
     * @brief définit les assets à ajouter au nouveau message en cours de préparation
     * @param assets liste d'assets.
     */
    void setAssets(const QVariantList& assets) { currentAssets_ = assets; }

    const QVariantList& getAssets() const { return currentAssets_; }
    
    /**
     * @brief Met à jour le contenu du chat
     * @param content Nouveau contenu à ajouter
     */
    virtual void updateContent(const QString& content) = 0;
    
    /**
     * @brief Met à jour le flux courant de l'IA
     * @param text Texte à ajouter au flux courant
     */
    virtual void updateCurrentAIStream(const QString& text) 
    {
        emit contextSizeUsedChanged();
        emit messagesChanged();
        emit historyChanged();        
        emit streamUpdated(text);
    };

    /**
     * @brief Retourne le nom du chat
     * @return Nom du chat
     */
    const QString& getName() const { return name_; }

    /**
     * @brief Retourne les services LLM associés
     * @return Pointeur vers les services LLM
     */
    LLMServices* getLLMServices() const { return llmservices_; }
    
    /**
     * @brief Retourne si le streaming est activé
     * @return true si le streaming est activé, false sinon
     */
    bool getStreamed() const { return streamed_; }

    /**
     * @brief Retourne l'historique des messages
     * @return Référence vers la liste des messages structurés
     */
    QList<ChatMessage>& getHistory() { return history_; }
    const QList<ChatMessage>& getHistory() const { return history_; }

    /**
     * @brief Retourne l'historique des messages formaté
     * @return QString contenant l'historique des messages formaté
     */
    virtual QString getFormattedHistory() const { return {}; }

    /**
     * @brief Retourne le premier message formaté correspondant au role à partir de l'index.
     * @param role Rôle du message à rechercher (user, assistant, system, thought, etc.)
     * @param index Index de départ pour la recherche :
     *              - Index positif (>= 0) : recherche depuis le début de l'historique (0 = premier message, 1 = deuxième, etc.)
     *              - Index négatif (< 0) : recherche depuis la fin de l'historique (-1 = dernier message, -2 = avant-dernier, etc.)
     *              L'index est automatiquement limité aux bornes valides de l'historique.
     * @return QString contenant le message formaté, ou QString vide si aucun message correspondant n'est trouvé
     */
    virtual QString getFormattedMessage(const QString& role, qsizetype index) const { return {}; }

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
    int getContextSize() const { return getData()->n_ctx_; };
    
    /**
     * @brief Retourne le nombre de tokens utilisés dans le contexte
     * @return Nombre de tokens utilisés
     */
    int getContextSizeUsed() const { return getData()->n_ctx_used_; };

    /**
     * @brief Retourne les données de chat
     * @return Pointeur vers les données de chat
     */
    ChatData* getData() { return dataPtr_ ? dataPtr_ : &data_; }
    const ChatData* getData() const { return dataPtr_ ? dataPtr_ : &data_; }

    /**
     * @brief Définit les données de chat
     * @param data Pointeur vers les données de chat
     */
    void setData(ChatData* data) 
    {
        data->n_ctx_ = data_.n_ctx_;
        data->n_ctx_used_ = data_.n_ctx_used_;
        data->context_tokens_ = data_.context_tokens_;
        dataPtr_ = data; 
    }

    /**
     * @brief Définit la taille du contexte
     * @param size Nouvelle taille du contexte
     */
    void setContextSize(int size)
    { 
        if (getData()->n_ctx_ != size)
        {
            qDebug() << "Chat::setContextSize" << size;
            getData()->n_ctx_ = size;
            getData()->reset();
            emit contextSizeChanged();
        }
    }

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

    /**
     * @brief Signal émis lorsque la taille du contexte change
     */
    void contextSizeChanged();

    /**
     * @brief Signal émis lorsque la consommation du contexte change
     */
    void contextSizeUsedChanged();

protected:
    /**
     * @brief Finalise le flux de streaming
     * 
     * Méthode appelée lorsque le streaming se termine pour nettoyer.
     */
    virtual void finalizeStream() = 0;

    // Data members
    ChatData data_;                 ///< Données de contexte du chat
    ChatData* dataPtr_{nullptr};    ///< Pointeur vers les données de contexte du chat
 
    bool streamed_{false};          ///< Indique si le streaming est activé
    bool processing_{false};        ///< Indique si le chat est en cours de traitement

    QString id_;                    ///< Identifiant stable du chat (UUID)
    QString name_;                  ///< Nom du chat
    QString currentApi_;            ///< API LLM courante
    QString currentModel_;          ///< Modèle LLM courant
    QString initialContext_;        ///< Contexte initial du chat

    QStringList messages_;          ///< Liste des messages sous forme de texte
    QList<ChatMessage> history_;    ///< Historique des messages structurés
    QJsonObject info_;              ///< Informations supplémentaires en JSON

    QVariantList currentAssets_;    ///< Liste des assets à ajouter au dernier message

    LLMServices* llmservices_{nullptr};   ///< Services LLM utilisés par ce chat
};
