#pragma once

#include "LLMServices.h"
#include "llama-cpp.h"

struct LlamaCppChatData;

/**
 * @struct LlamaCppProcess
 * @brief Processus de traitement pour Llama.cpp
 * 
 * Cette structure définit l'interface pour les processus de génération
 * utilisant Llama.cpp. Elle gère le cycle de vie du traitement.
 */
struct LlamaCppProcess
{
    /**
     * @brief Constructeur de LlamaCppProcess
     * @param type Type de processus
     * @param data Données du chat associées
     * @param service Service LLM parent
     */
    LlamaCppProcess(int type, LlamaCppChatData* data, LLMServices* service) : type_(type), data_(data), service_(service)
    {
    }
    
    /**
     * @brief Destructeur virtuel
     */
    virtual ~LlamaCppProcess() {}

    /**
     * @brief Démarre le processus de génération
     * @param chat Chat associé
     * @param content Contenu à générer
     * @param streamed Indique si le streaming est activé
     */
    virtual void start(Chat* chat, const QString& content, bool streamed) = 0;
    
    /**
     * @brief Arrête le processus
     */
    virtual void stop() = 0;
    
    /**
     * @brief Arrête le processus sous-jacent
     */
    virtual void stopProcess() = 0;
    
    /**
     * @brief Génère du contenu (méthode par défaut vide)
     */
    virtual void generate() {}

    int type_;                    ///< Type de processus
    LlamaCppChatData* data_;      ///< Données du chat associées
    LLMServices* service_;        ///< Service LLM parent
};

/**
 * @struct LlamaModelData
 * @brief Données d'un modèle Llama.cpp
 * 
 * Stocke les informations et l'état d'un modèle Llama.cpp chargé.
 */
struct LlamaModelData
{
    QString modelName_;           ///< Nom du modèle
    QString modelPath_;           ///< Chemin vers le fichier du modèle
    int n_gpu_layers_{99};        ///< Nombre de couches à charger sur GPU (99 = toutes)
    bool use_gpu_{true};          ///< Activer/désactiver GPU
    llama_model* model_{nullptr};  ///< Pointeur vers le modèle llama.cpp
};

/**
 * @struct LlamaCppChatData
 * @brief Données de chat spécifiques à Llama.cpp
 * 
 * Étend ChatData avec des informations spécifiques à Llama.cpp
 * pour la gestion des chats et de la génération.
 */
struct LlamaCppChatData : public ChatData
{
    /**
     * @brief Constructeur par défaut
     */
    LlamaCppChatData() = default;
    
    /**
     * @brief Destructeur
     * 
     * Nettoie les ressources allouées par llama.cpp.
     */
    ~LlamaCppChatData();

    /**
     * @brief Initialise les données du chat
     * @param model Modèle à utiliser (optionnel)
     */
    void initialize(LlamaModelData* model = nullptr);
    
    /**
     * @brief Désinitialise les données du chat
     * 
     * Libère les ressources et réinitialise l'état.
     */
    void deinitialize();

    /**
     * @brief Réinitialise les données du chat
     * 
     * Réinitialise l'état sans libérer les ressources.
     */
    void reset() override;

    /**
     * @brief Vide les données du chat
     * 
     * Vide les données sans libérer les ressources.
     */
    void clear();

    Chat* chat_{nullptr};                     ///< Pointeur vers le chat associé

    QString response_;                         ///< Réponse courante

    LlamaModelData* model_{nullptr};          ///< Modèle utilisé
    llama_context* ctx_{nullptr};             ///< Contexte llama.cpp
    llama_sampler* smpl_{nullptr};            ///< Échantillonneur llama.cpp
    const char* llamaCppChattemplate_{nullptr}; ///< Template de chat

    llama_batch batch_;                       ///< Batch de traitement
    llama_token tokenId_;                     ///< ID du token courant

    std::vector<llama_token> tokens_;         ///< Tokens générés
    std::vector<llama_chat_message> llamaCppChatMessages_; ///< Messages de chat

    LlamaCppProcess* generateProcess_{nullptr}; ///< Processus de génération
};

/**
 * @class LlamaCppWorker
 * @brief Worker pour le traitement asynchrone avec Llama.cpp
 * 
 * Cette classe gère l'exécution des processus Llama.cpp dans
 * un thread séparé pour éviter de bloquer l'interface utilisateur.
 */
class LlamaCppWorker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de LlamaCppWorker
     * @param process Processus à exécuter
     */
    explicit LlamaCppWorker(LlamaCppProcess* process);
    
    /**
     * @brief Destructeur de LlamaCppWorker
     * 
     * Nettoie le thread et les ressources.
     */
    ~LlamaCppWorker();

public slots:
    /**
     * @brief Traite la requête
     * 
     * Méthode appelée pour démarrer le traitement.
     */
    void processRequest();
    
    /**
     * @brief Arrête le traitement
     * 
     * Interrompt le processus de génération.
     */
    void stopProcessing();

signals:
    /**
     * @brief Signal émis lorsqu'un token est généré
     * @param token Token généré
     */
    void tokenGenerated(const QString& token);
    
    /**
     * @brief Signal émis lorsque la génération est terminée
     */
    void generationFinished();
    
    /**
     * @brief Signal émis lorsqu'une erreur se produit
     * @param error Message d'erreur
     */
    void errorOccurred(const QString& error);

private:
    LlamaCppProcess* process_;   ///< Processus à exécuter
    QThread* thread_;            ///< Thread d'exécution
};

/**
 * @class LlamaCppService
 * @brief Service LLM utilisant Llama.cpp
 * 
 * Cette classe implémente LLMService en utilisant la bibliothèque
 * Llama.cpp pour l'exécution locale des modèles de langage.
 * 
 * Elle gère le chargement des modèles, la génération de texte,
 * et la gestion des chats avec support GPU.
 */
class LlamaCppService : public LLMService
{
    Q_OBJECT

public:
    /**
     * @brief Constructeur de LlamaCppService
     * @param service Service LLM parent
     * @param name Nom du service
     */
    LlamaCppService(LLMServices* service, const QString& name);
    
    /**
     * @brief Constructeur de LlamaCppService avec paramètres
     * @param service Service LLM parent
     * @param params Paramètres de configuration
     */
    LlamaCppService(LLMServices* service, const QVariantMap& params);
    
    /**
     * @brief Destructeur de LlamaCppService
     * 
     * Nettoie les modèles chargés et les ressources.
     */
    ~LlamaCppService() override;

    /**
     * @brief Définit le modèle pour un chat
     * @param chat Chat cible
     * @param model Nom du modèle (optionnel)
     */
    void setModel(Chat* chat, QString model = "") override;

    /**
     * @brief Vérifie si le service est prêt
     * @return true si le service est prêt, false sinon
     */
    bool isReady() const override;

    /**
     * @brief Envoie une requête à générer
     * @param chat Chat associé
     * @param content Contenu à générer
     * @param streamed Indique si le streaming est activé (par défaut: true)
     */
    void post(Chat* chat, const QString& content, bool streamed = true) override;
    
    /**
     * @brief Formate les messages pour Llama.cpp
     * @param chat Chat associé
     * @return Messages formatés
     */
    QString formatMessages(Chat* chat) override;
    
    /**
     * @brief Arrête le streaming pour un chat
     * @param chat Chat à arrêter
     */
    void stopStream(Chat* chat) override;

    /**
     * @brief Crée des données de chat pour Llama.cpp
     * @param chat Chat associé
     * @return Pointeur vers les données créées
     */
    LlamaCppChatData* createData(Chat* chat);
    
    /**
     * @brief Initialise les données de chat
     * @param data Données à initialiser
     * @param model Modèle à utiliser (optionnel)
     */
    void initializeData(LlamaCppChatData* data, LlamaModelData* model = nullptr);
    
    /**
     * @brief Efface les données de chat
     * @param data Données à effacer
     */
    void clearData(LlamaCppChatData* data);
    
    /**
     * @brief Efface un modèle de la mémoire
     * @param modelName Nom du modèle à effacer
     */
    void clearModelInMemory(const QString& modelName);

    // Activer/désactiver la version threadée
    /**
     * @brief Active ou désactive la version threadée
     * @param useThreaded true pour activer, false pour désactiver
     */
    void setUseThreadedVersion(bool useThreaded) { useThreadedVersion_ = useThreaded; }
    
    /**
     * @brief Retourne si la version threadée est utilisée
     * @return true si la version threadée est utilisée, false sinon
     */
    bool isUsingThreadedVersion() const { return useThreadedVersion_; }

    // Configuration GPU
    /**
     * @brief Définit le nombre de couches GPU par défaut
     * @param n_gpu_layers Nombre de couches GPU
     */
    void setDefaultGpuLayers(int n_gpu_layers);
    
    /**
     * @brief Définit la taille de contexte par défaut
     * @param n_ctx Taille de contexte
     */
    void setDefaultContextSize(int n_ctx);
    
    /**
     * @brief Active ou désactive l'utilisation du GPU par défaut
     * @param use_gpu true pour activer le GPU, false pour le désactiver
     */
    void setDefaultUseGpu(bool use_gpu);

    /**
     * @brief Retourne le nombre de couches GPU pour un chat
     * @param chat Chat spécifique (optionnel)
     * @return Nombre de couches GPU
     */
    int getGpuLayers(Chat* chat = nullptr) const;
    
    /**
     * @brief Retourne la taille de contexte pour un chat
     * @param chat Chat spécifique (optionnel)
     * @return Taille de contexte
     */
    int getContextSize(Chat* chat = nullptr) const;
    
    /**
     * @brief Retourne si le GPU est utilisé pour un chat
     * @param chat Chat spécifique (optionnel)
     * @return true si le GPU est utilisé, false sinon
     */
    bool isUsingGpu(Chat* chat = nullptr) const;

    /**
     * @brief Retourne un modèle par nom
     * @param modelname Nom du modèle
     * @return Pointeur vers les données du modèle
     */
    LlamaModelData* getModel(const QString& modelname);
    
    /**
     * @brief Retourne un modèle par nom (version const)
     * @param modelname Nom du modèle
     * @return Pointeur constant vers les données du modèle
     */
    const LlamaModelData* getModel(const QString& modelname) const;
    
    /**
     * @brief Retourne la liste des modèles disponibles
     * @return Vecteur contenant les modèles disponibles
     */
    std::vector<LLMModel> getAvailableModels() const override;
    
    /**
     * @brief Retourne les données de chat pour un chat donné
     * @param chat Chat cible
     * @return Pointeur vers les données de chat
     */
    LlamaCppChatData* getData(Chat* chat);
    
    /**
     * @brief Retourne les données de chat pour un chat donné (version const)
     * @param chat Chat cible
     * @return Pointeur constant vers les données de chat
     */
    const LlamaCppChatData* getData(const Chat* chat) const;

    /**
     * @brief Génère un embedding pour un texte
     * @param text Texte à encoder
     * @return Vecteur contenant l'embedding
     */
    std::vector<float> getEmbedding(const QString& text) override;

    // Informations sur les backends disponibles
    /**
     * @brief Retourne la liste des backends disponibles
     * @return Liste des backends supportés
     */
    static QStringList getAvailableBackends();
    
    /**
     * @brief Retourne des informations sur les backends
     * @return Informations sur les backends
     */
    static QString getBackendInfo();
    
    /**
     * @brief Crée une instance par défaut de LlamaCppService
     * @param service Service LLM parent
     * @param name Nom du service
     * @return Pointeur vers le service créé
     */
    static LlamaCppService* createDefault(LLMServices* service, const QString& name);

    QHash<QString, LlamaModelData> models_;          ///< Modèles chargés
    QHash<const Chat*, LlamaCppChatData> datas_;     ///< Données de chat

    int defaultGpuLayers_{99};                       ///< Couches GPU par défaut
    int defaultContextSize_{LLM_DEFAULT_CONTEXT_SIZE}; ///< Taille de contexte par défaut
    bool defaultUseGpu_{true};                       ///< Utilisation GPU par défaut
    bool useThreadedVersion_{false};                 ///< Version threadée activée
    bool onlyOneModelInMemory_{true};                ///< Un seul modèle en mémoire

private:
    /**
     * @brief Charge un modèle en mémoire
     * @param model Nom du modèle
     * @param numGpuLayers Nombre de couches GPU
     * @param clearOtherModels Effacer les autres modèles
     * @return Pointeur vers les données du modèle chargé
     */
    LlamaModelData* loadModel(const QString& model, int numGpuLayers, bool clearOtherModels);
    
    /**
     * @brief Définit un modèle interne pour des données de chat
     * @param data Données de chat
     * @param modelName Nom du modèle
     */
    void setModelInternal(LlamaCppChatData* data, const QString& modelName);

    LlamaModelData* lastModelAddedInMemory_{nullptr};  ///< Dernier modèle chargé
    LlamaModelData* embeddingModel_{nullptr};          ///< Modèle pour les embeddings
};
