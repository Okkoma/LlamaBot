#pragma once

#include "LLMServices.h"
#include "RAGService.h"
#include "ChatStorage.h"

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
    Q_PROPERTY(int defaultContextSize READ getDefaultContextSize WRITE setDefaultContextSize NOTIFY defaultContextSizeChanged)
    Q_PROPERTY(bool autoExpandContext READ getAutoExpandContext WRITE setAutoExpandContext NOTIFY autoExpandContextChanged)
    Q_PROPERTY(QVariantList pendingAssets READ pendingAssets NOTIFY pendingAssetsChanged)
    Q_PROPERTY(QString currentModel READ currentModel WRITE setCurrentModel NOTIFY currentModelChanged)
    Q_PROPERTY(QString currentApi READ currentApi WRITE setCurrentApi NOTIFY currentApiChanged)

    QML_ELEMENT
    QML_SINGLETON
    QML_UNCREATABLE("ChatController is a singleton provided by the application")

public:
    explicit ChatController(QObject* parent = nullptr);

    void initialize();
    
    /**
     * @brief Destructeur du contrôleur de chat
     * 
     * Nettoie les ressources et sauvegarde l'état des chats.
     */
    ~ChatController() = default;

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
    Q_INVOKABLE void createChat(const QString& api, const QString& model);
    
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

    // Chat Serialization
    /**
     * @brief Sauvegarde les chats dans un fichier
     *
     * Sérialise et sauvegarde l'état des chats sur le disque.
     */
    Q_INVOKABLE void saveChats(bool sync=false);

    /**
     * @brief Charge les chats depuis un fichier
     *
     * Charge et désérialise l'état des chats depuis le disque.
     */
    Q_INVOKABLE void loadChats();

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
     * @brief Retourne le modele courant
     * @return le nom du modele courant
     */
    const QString& currentModel() const { return currentModel_; }

    Q_INVOKABLE int currentModelIndex() const;

    /**
     * @brief Définit le nom du modèle selectionné dans l'ui à utiliser par défaut
     * @param modelName Nom du modèle selectionné
     */    
    Q_INVOKABLE void setCurrentModel(const QString& modelname=QString());

    /**
     * @brief Retourne la liste des modèles disponibles
     * @return Liste des modèles LLM disponibles
     */
    Q_INVOKABLE QVariantList getAvailableModels();
    
    /**
     * @brief Retourne la liste des APIs disponibles
     * @return Liste des APIs LLM disponibles
     */
    Q_INVOKABLE QVariantList getAvailableAPIs() const;
    
    Q_INVOKABLE QVariantMap getAPI(const QString& apiname) const;

    const QString& currentApi() const { return currentAPI_; }

    Q_INVOKABLE int currentApiIndex() const;

    /**
     * @brief Définit le nom du service selectionné dans l'ui à utiliser par défaut
     * @param apiName Nom du service selectionné
     */    
    Q_INVOKABLE void setCurrentApi(const QString& apiName=QString());

    /**
     * @brief Retourne la liste des types d'APIs 
     * @return Liste des types d'APIs disponibles
     */
    Q_INVOKABLE QVariantList getRegisteredAPITypes() const;

    /**
     * @brief Définit le modèle LLM à utiliser
     * @param modelName Nom du modèle à utiliser
     */
    Q_INVOKABLE void setModel(const QString& modelName);
    
    /**
     * @brief Supprime un modele
     * @param index Index du modele à supprimer
     */
    Q_INVOKABLE void deleteModel(int index);

    /**
     * @brief Définit l'API LLM à utiliser
     * @param apiName Nom de l'API à utiliser
     */
    Q_INVOKABLE void setAPI(const QString& apiName);

    /**
     * @brief Ajoute/Modifie une API
     * @param apiName Nom de l'API à utiliser
     * @param apiType type de l'API
     * @param url     url de l'API
     * @param apikey  apikey
     * @return false si echec.
     */
    Q_INVOKABLE bool modifyAPI(const QString& apiName, const QString& apiType, const QString& url, const QString& apikey);

    /**
     * @brief Rafraîchit la liste des modèles disponibles
     * 
     * Émet le signal availableModelsChanged() pour notifier que la liste des modèles
     * doit être mise à jour (par exemple après un téléchargement).
     */
    Q_INVOKABLE void refreshModels();

    Q_INVOKABLE void resetSettings();

    /**
     * @brief Ajoute des assets au chat courant
     * @param urls liste des urls
     */
    Q_INVOKABLE void addAssets(const QStringList& urls);

    /**
     * @brief Ajoute un asset au chat courant
     * @param assetContent contenu de l'asset base64
     */
    Q_INVOKABLE void addAssetBase64(const QString& assetContent);

    Q_INVOKABLE QString getMimeTypeIconFor(const QString& fileName) const;

    Q_INVOKABLE void removeAsset(int index);
    Q_INVOKABLE void clearAssets();
    QVariantList pendingAssets() const { return pendingAssets_; }
    
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
     * @brief Signal émis lorsque la liste des APIs disponibles change
     */
    void availableAPIsChanged();
    
    /**
     * @brief Signal émis lorsque le model courant change
     */
     void currentModelChanged();

    /**
     * @brief Signal émis lorsque le service courant change
     */
     void currentApiChanged();
     
    /**
     * @brief Signal émis lorsque le chargement commence
     */
    void loadingStarted();
    
    /**
     * @brief Signal émis lorsque le chargement se termine
     */
    void loadingFinished();

    void pendingAssetsChanged();

private:
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

    void addFileAsset(const QString& filePath);
    
    LLMServices* llmServices_;    ///< Service LLM pour les opérations de chat
    ChatStorage* localStore_;     ///< Stockage local (SQLite)
    ChatStorage* cloudStore_;     ///< Stockage cloud (Psql)
    QList<Chat*> chats_;          ///< Liste des chats gérés par le contrôleur
    Chat* currentChat_;           ///< Chat actuellement sélectionné
    int chatCounter_;             ///< Compteur pour générer des noms de chat uniques
    QVariantList pendingAssets_;  ///< Liste temporaire des assets en attente
    QString currentModel_;        ///< nom du modele courant
    QString currentAPI_;          ///< nom du service courant

public:
    int getDefaultContextSize() const { return llmServices_->getDefaultContextSize(); }
    void setDefaultContextSize(int size);
    bool getAutoExpandContext() const { return llmServices_->getAutoExpandContext(); }
    void setAutoExpandContext(bool enabled);

signals:
    /**
     * @brief Signal émis lorsque la taille de contexte par défaut change
     */
    void defaultContextSizeChanged();

    /**
     * @brief Signal émis lorsque l'auto-expansion du contexte change
     */
    void autoExpandContextChanged();
};
