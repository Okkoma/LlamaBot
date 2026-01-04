#pragma once

#include "LLMServices.h"
#include "RAGService.h"

/**
 * @class ChatController
 * @brief Contrôleur principal pour la gestion des chats et conversations
 * 
 * Cette classe gère la création, la suppression et la commutation entre les chats,
 * ainsi que l'envoi et la réception de messages via les services LLM.
 * 
 * Elle fournit une interface QML via des propriétés et méthodes invokables.
 */
class ChatController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Chat* currentChat READ currentChat NOTIFY currentChatChanged)
    Q_PROPERTY(QVariantList chatList READ chatList NOTIFY chatListChanged)
    Q_PROPERTY(int currentChatIndex READ currentChatIndex NOTIFY currentChatChanged)
    Q_PROPERTY(bool ragEnabled READ ragEnabled WRITE setRagEnabled NOTIFY ragEnabledChanged)
    Q_PROPERTY(RAGService* ragService READ ragService CONSTANT)
    Q_PROPERTY(int defaultContextSize READ getDefaultContextSize WRITE setDefaultContextSize NOTIFY defaultContextSizeChanged)
    Q_PROPERTY(bool autoExpandContext READ getAutoExpandContext WRITE setAutoExpandContext NOTIFY autoExpandContextChanged)

public:
    /**
     * @brief Constructeur du contrôleur de chat
     * @param service Service LLM à utiliser pour les opérations de chat
     * @param parent Objet parent Qt (optionnel)
     */
    explicit ChatController(LLMServices* service, QObject* parent = nullptr);
    
    /**
     * @brief Destructeur du contrôleur de chat
     * 
     * Nettoie les ressources et sauvegarde l'état des chats.
     */
    ~ChatController();

    /**
     * @brief Retourne le chat courant
     * @return Pointeur vers le chat actuellement sélectionné
     */
    Chat* currentChat() const { return currentChat_; }
    
    /**
     * @brief Retourne la liste des chats
     * @return Liste des chats sous forme de QVariantList pour QML
     */
    QVariantList chatList() const;
    
    /**
     * @brief Retourne l'index du chat courant
     * @return Index du chat actuellement sélectionné
     */
    int currentChatIndex() const;

    // Chat Management
    /**
     * @brief Crée un nouveau chat
     * 
     * Crée un nouveau chat vide et le sélectionne comme chat courant.
     */
    Q_INVOKABLE void createChat();
    
    /**
     * @brief Change de chat courant
     * @param index Index du chat à sélectionner
     */
    Q_INVOKABLE void switchToChat(int index);
    
    /**
     * @brief Supprime un chat
     * @param index Index du chat à supprimer
     */
    Q_INVOKABLE void deleteChat(int index);
    
    /**
     * @brief Renomme un chat
     * @param index Index du chat à renommer
     * @param name Nouveau nom du chat
     */
    Q_INVOKABLE void renameChat(int index, const QString& name);

    // Message Operations
    /**
     * @brief Envoie un message dans le chat courant
     * @param text Texte du message à envoyer
     */
    Q_INVOKABLE void sendMessage(const QString& text);
    
    /**
     * @brief Arrête la génération de message en cours
     * 
     * Interrompt la génération de réponse par le modèle LLM.
     */
    Q_INVOKABLE void stopGeneration();

    // Model Management
    /**
     * @brief Retourne la liste des modèles disponibles
     * @return Liste des modèles LLM disponibles
     */
    Q_INVOKABLE QVariantList getAvailableModels();
    
    /**
     * @brief Retourne la liste des APIs disponibles
     * @return Liste des APIs LLM disponibles
     */
    Q_INVOKABLE QVariantList getAvailableAPIs();
    
    /**
     * @brief Définit le modèle LLM à utiliser
     * @param modelName Nom du modèle à utiliser
     */
    Q_INVOKABLE void setModel(const QString& modelName);
    
    /**
     * @brief Définit l'API LLM à utiliser
     * @param apiName Nom de l'API à utiliser
     */
    Q_INVOKABLE void setAPI(const QString& apiName);

signals:
    /**
     * @brief Signal émis lorsque le contenu d'un chat est mis à jour
     * @param chat Chat qui a été mis à jour
     */
    void chatContentUpdated(Chat* chat);
    
    /**
     * @brief Signal émis lorsque le chat courant change
     */
    void currentChatChanged();
    
    /**
     * @brief Signal émis lorsque la liste des chats change
     */
    void chatListChanged();
    
    /**
     * @brief Signal émis lorsque la liste des modèles disponibles change
     */
    void availableModelsChanged();
    
    /**
     * @brief Signal émis lorsque le chargement commence
     */
    void loadingStarted();
    
    /**
     * @brief Signal émis lorsque le chargement se termine
     */
    void loadingFinished();

private:
    /**
     * @brief Sauvegarde les chats dans un fichier
     * 
     * Sérialise et sauvegarde l'état des chats sur le disque.
     */
    void saveChats();
    
    /**
     * @brief Charge les chats depuis un fichier
     * 
     * Charge et désérialise l'état des chats depuis le disque.
     */
    void loadChats();
    
    /**
     * @brief Retourne le chemin du fichier de sauvegarde des chats
     * @return Chemin complet du fichier de sauvegarde
     */
    QString getChatsFilePath() const;
    
    /**
     * @brief Notifie la mise à jour d'un chat
     * @param chat Chat qui a été mis à jour
     */
    void notifyUpdatedChat(Chat* chat);
    
    /**
     * @brief Vérifie si le traitement des chats est terminé
     * 
     * Émet le signal loadingFinished si tous les chats ont terminé leur traitement.
     */
    void checkChatsProcessingFinished();
    
    /**
     * @brief Connecte les signaux des APIs
     * 
     * Établit les connexions entre les signaux des services LLM et les slots du contrôleur.
     */
    void connectAPIsSignals();

    LLMServices* llmServices_;    ///< Service LLM pour les opérations de chat
    RAGService* ragService_;      ///< Service RAG pour la recherche augmentée
    QList<Chat*> chats_;          ///< Liste des chats gérés par le contrôleur
    Chat* currentChat_;          ///< Chat actuellement sélectionné
    int chatCounter_;             ///< Compteur pour générer des noms de chat uniques
    bool ragEnabled_ = false;    ///< Indique si le RAG est activé

public:
    /**
     * @brief Retourne l'état du RAG
     * @return true si le RAG est activé, false sinon
     */
    bool ragEnabled() const { return ragEnabled_; }
    
    /**
     * @brief Active ou désactive le RAG
     * @param enabled État souhaité pour le RAG
     */
    void setRagEnabled(bool enabled)
    {
        if (ragEnabled_ != enabled)
        {
            ragEnabled_ = enabled;
            emit ragEnabledChanged();
        }
    }
    
    /**
     * @brief Retourne le service RAG
     * @return Pointeur vers le service RAG
     */
    RAGService* ragService() const { return ragService_; }

    int getDefaultContextSize() const { return llmServices_->getDefaultContextSize(); }
    void setDefaultContextSize(int size);
    bool getAutoExpandContext() const { return llmServices_->getAutoExpandContext(); }
    void setAutoExpandContext(bool enabled);

signals:
    /**
     * @brief Signal émis lorsque l'état du RAG change
     */
    void ragEnabledChanged();

    /**
     * @brief Signal émis lorsque la taille de contexte par défaut change
     */
    void defaultContextSizeChanged();

    /**
     * @brief Signal émis lorsque l'auto-expansion du contexte change
     */
    void autoExpandContextChanged();
};
