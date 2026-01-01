#pragma once

#include "LLMService.h"
#include "Chat.h"

/**
 * @class LLMServices
 * @brief Service de gestion des APIs LLM
 * 
 * Cette classe gère plusieurs services LLM (Ollama, Llama.cpp, etc.)
 * et fournit une interface unifiée pour interagir avec différents
 * modèles de langage.
 * 
 * Elle gère le cycle de vie des services, le partage des modèles,
 * et la communication avec les différentes APIs.
 */
class LLMServices : public QObject
{
    Q_OBJECT

    friend class RAGService;

public:
    /**
     * @brief Constructeur de LLMServices
     * @param parent Objet parent Qt
     */
    explicit LLMServices(QObject* parent);
    
    /**
     * @brief Destructeur de LLMServices
     * 
     * Nettoie les ressources et les services LLM.
     */
    ~LLMServices();

    /**
     * @brief Active ou désactive le partage des modèles entre services
     * @param enable true pour activer le partage, false pour le désactiver
     */
    void allowSharedModels(bool enable);
    
    /**
     * @brief Ajoute une API LLM au service
     * @param api Pointeur vers l'API LLM à ajouter
     */
    void addAPI(LLMService* api);

    /**
     * @brief Arrête le traitement pour un chat donné
     * @param chat Chat à arrêter
     */
    void stop(Chat* chat);
    
    /**
     * @brief Envoie une requête à une API LLM
     * @param api API LLM cible
     * @param chat Chat associé à la requête
     * @param content Contenu de la requête
     * @param streamed Indique si la réponse doit être streamée (par défaut: true)
     */
    void post(LLMService* api, Chat* chat, const QString& content, bool streamed = true);
    
    /**
     * @brief Reçoit des données d'une API LLM
     * @param api API LLM source
     * @param chat Chat associé
     * @param data Données reçues
     */
    void receive(LLMService* api, Chat* chat, const QByteArray& data);

    /**
     * @brief Retourne si le partage des modèles est activé
     * @return true si le partage est activé, false sinon
     */
    bool hasSharedModels() const { return allowSharedModels_; }
    
    /**
     * @brief Vérifie si un service LLM est disponible
     * @param service Type de service LLM
     * @return true si le service est disponible, false sinon
     */
    bool isServiceAvailable(LLMEnum::LLMType service) const;
    
    /**
     * @brief Retourne un service LLM par type
     * @param service Type de service LLM
     * @return Pointeur vers le service LLM, ou nullptr si non trouvé
     */
    LLMService* get(LLMEnum::LLMType service) const;
    
    /**
     * @brief Retourne un service LLM par nom
     * @param name Nom du service LLM
     * @return Pointeur vers le service LLM, ou nullptr si non trouvé
     */
    LLMService* get(const QString& name) const;
    
    /**
     * @brief Retourne la liste de toutes les APIs LLM
     * @return Référence vers le vecteur contenant toutes les APIs
     */
    const std::vector<LLMService*>& getAPIs() const;
    
    /**
     * @brief Retourne la liste des APIs LLM disponibles
     * @return Vecteur contenant les APIs disponibles
     */
    std::vector<LLMService*> getAvailableAPIs() const;  
    
    /**
     * @brief Retourne la liste des modèles disponibles
     * @param api API LLM spécifique (optionnel)
     * @return Vecteur contenant les modèles disponibles
     */
    std::vector<LLMModel> getAvailableModels(const LLMService* api = nullptr);
    
    /**
     * @brief Retourne un modèle LLM par nom
     * @param name Nom du modèle
     * @return Modèle LLM correspondant
     */
    LLMModel getModel(const QString& name) const;
    
    /**
     * @brief Génère un embedding pour un texte
     * @param text Texte à encoder
     * @return Vecteur contenant l'embedding
     */
    std::vector<float> getEmbedding(const QString& text);

    /**
     * @brief Charge la configuration des services depuis un fichier JSON
     * @return true si le chargement a réussi, false sinon
     */
    bool loadServiceJsonFile();
    
    /**
     * @brief Sauvegarde la configuration des services dans un fichier JSON
     * @return true si la sauvegarde a réussi, false sinon
     */
    bool saveServiceJsonFile();

private:
    /**
     * @brief Initialise les services LLM
     * 
     * Crée et configure les services par défaut.
     */
    void initialize();
    
    /**
     * @brief Crée un fichier JSON de configuration par défaut
     * 
     * Génère un fichier de configuration avec les paramètres par défaut.
     */
    void createDefaultServiceJsonFile();

    /**
     * @brief Vérifie si un service nécessite un processus séparé
     * @param api API LLM à vérifier
     * @return true si un processus séparé est nécessaire, false sinon
     */
    bool requireStartProcess(LLMService* api);
    
    /**
     * @brief Gère les erreurs de message
     * @param chat Chat concerné
     * @param message Message d'erreur
     */
    void handleMessageError(Chat* chat, const QString& message);

    std::vector<LLMService*> apiEntries_;    ///< Liste des APIs LLM enregistrées
    bool allowSharedModels_;                 ///< Indique si le partage des modèles est activé
};