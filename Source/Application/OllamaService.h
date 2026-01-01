#pragma once

#include "LLMServices.h"

class QNetworkAccessManager;

/**
 * @class OllamaManifest
 * @brief Manifest pour les modèles Ollama
 * 
 * Cette classe représente le manifest d'un modèle Ollama,
 * contenant les métadonnées et les informations sur les couches.
 */
class OllamaManifest
{
public:
    /**
     * @brief Constructeur par défaut
     */
    OllamaManifest() = default;

    /**
     * @brief Retourne le nom du fichier du modèle
     * @return Nom du fichier du modèle
     * 
     * Extrait le nom du fichier à partir des couches du manifest.
     */
    QString getModelFileName() const
    {
        for (const Layer& layer : layers_)
        {
            if (layer.mediaType_.contains("ollama.image.model"))
            {
                QString name = layer.digest_;
                return name.replace(':', '-');
            }
        }
        return QString("");
    }

    /**
     * @brief Crée un OllamaManifest à partir d'un objet JSON
     * @param obj Objet JSON contenant les données du manifest
     * @return OllamaManifest créé
     */
    static OllamaManifest fromJson(const QJsonObject& obj)
    {
        OllamaManifest manifest;
        manifest.schemaVersion_ = obj["schemaVersion"].toInt();
        manifest.mediaType_ = obj["mediaType"].toString();
        if (obj.contains("config") && obj["config"].isObject())
        {
            QJsonObject configObj = obj["config"].toObject();
            manifest.config.mediaType_ = configObj["mediaType"].toString();
            manifest.config.digest_ = configObj["digest"].toString();
            manifest.config.size_ = configObj["size"].toVariant().toLongLong();
        }
        if (obj.contains("layers") && obj["layers"].isArray())
        {
            QJsonArray layersArray = obj["layers"].toArray();
            for (const QJsonValue& layerVal : layersArray)
            {
                if (layerVal.isObject())
                {
                    QJsonObject layerObj = layerVal.toObject();
                    Layer layer;
                    layer.mediaType_ = layerObj["mediaType"].toString();
                    layer.digest_ = layerObj["digest"].toString();
                    layer.size_ = layerObj["size"].toVariant().toLongLong();
                    manifest.layers_.append(layer);
                }
            }
        }
        return manifest;
    }

    /**
     * @brief Convertit le manifest en objet JSON
     * @return Objet JSON représentant le manifest
     */
    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["schemaVersion"] = schemaVersion_;
        obj["mediaType"] = mediaType_;
        QJsonObject configObj;
        configObj["mediaType"] = config.mediaType_;
        configObj["digest"] = config.digest_;
        configObj["size"] = QString::number(config.size_).toLongLong();
        obj["config"] = configObj;
        QJsonArray layersArray;
        for (const Layer& layer : layers_)
        {
            QJsonObject layerObj;
            layerObj["mediaType"] = layer.mediaType_;
            layerObj["digest"] = layer.digest_;
            layerObj["size"] = QString::number(layer.size_).toLongLong();
            layersArray.append(layerObj);
        }
        obj["layers"] = layersArray;
        return obj;
    }

    QString model_;           ///< Nom du modèle
    QString num_params_;      ///< Nombre de paramètres

    int schemaVersion_;       ///< Version du schéma
    QString mediaType_;       ///< Type de média
    
    /**
     * @struct Config
     * @brief Configuration du manifest
     */
    struct Config
    {
        QString mediaType_;    ///< Type de média de la configuration
        QString digest_;       ///< Digest de la configuration
        qint64 size_;          ///< Taille de la configuration
    } config;
    
    /**
     * @struct Layer
     * @brief Couche du modèle
     */
    struct Layer
    {
        QString mediaType_;    ///< Type de média de la couche
        QString digest_;       ///< Digest de la couche
        qint64 size_;          ///< Taille de la couche
    };
    QList<Layer> layers_;     ///< Liste des couches
};

/**
 * @class OllamaService
 * @brief Service LLM pour Ollama
 * 
 * Cette classe implémente LLMService pour interagir avec
 * le serveur Ollama. Elle gère la communication réseau,
 * le démarrage/arrêt du serveur, et l'exécution des modèles.
 */
class OllamaService : public LLMService
{
public:
    /**
     * @brief Constructeur de OllamaService
     * @param service Service LLM parent
     * @param name Nom du service
     * @param url URL du serveur Ollama
     * @param ver Version de l'API
     * @param gen Endpoint de génération
     * @param apiKey Clé API (optionnelle)
     * @param programPath Chemin vers l'exécutable Ollama
     * @param programArguments Arguments pour l'exécutable
     */
    OllamaService(LLMServices* service, const QString& name, const QString& url, const QString& ver, const QString& gen,
        const QString& apiKey, const QString& programPath, const QStringList& programArguments);
    
    /**
     * @brief Constructeur de OllamaService avec paramètres
     * @param service Service LLM parent
     * @param params Paramètres de configuration
     */
    OllamaService(LLMServices* service, const QVariantMap& params);
    
    /**
     * @brief Destructeur de OllamaService
     * 
     * Nettoie les ressources et arrête le serveur si nécessaire.
     */
    ~OllamaService() override;

    /**
     * @brief Démarre le service Ollama
     * @return true si le démarrage a réussi, false sinon
     */
    bool start() override;
    
    /**
     * @brief Arrête le service Ollama
     * @return true si l'arrêt a réussi, false sinon
     */
    bool stop() override;
    
    /**
     * @brief Vérifie si le service est prêt
     * @return true si le service est prêt, false sinon
     */
    bool isReady() const override;

    /**
     * @brief Envoie une requête à Ollama
     * @param chat Chat associé
     * @param content Contenu à générer
     * @param streamed Indique si le streaming est activé (par défaut: true)
     */
    void post(Chat* chat, const QString& content, bool streamed = true) override;

    /**
     * @brief Gère les erreurs de message
     * @param chat Chat concerné
     * @param message Message d'erreur
     * @return true si l'erreur a été gérée, false sinon
     */
    bool handleMessageError(Chat* chat, const QString& message) override;

    /**
     * @brief Vérifie si l'URL est accessible
     * @return true si l'URL est accessible, false sinon
     */
    bool isUrlAccessible() const;
    
    /**
     * @brief Vérifie si l'API est accessible
     * @return true si l'API est accessible, false sinon
     */
    bool isAPIAccessible() const;
    
    /**
     * @brief Vérifie si le processus est démarré
     * @return true si le processus est démarré, false sinon
     */
    bool isProcessStarted() const;
    
    /**
     * @brief Vérifie si le processus peut être démarré
     * @return true si le processus peut être démarré, false sinon
     */
    bool canStartProcess() const;

    /**
     * @brief Retourne la liste des modèles disponibles
     * @return Vecteur contenant les modèles disponibles
     */
    std::vector<LLMModel> getAvailableModels() const override;
    
    /**
     * @brief Retourne le manifest d'un modèle Ollama
     * @param ollamaDir Répertoire Ollama
     * @param model Nom du modèle
     * @param num_params Nombre de paramètres
     * @return Manifest du modèle
     */
    static OllamaManifest getOllamaManifest(const QString& ollamaDir, const QString& model, const QString& num_params);
    
    /**
     * @brief Retourne les manifests de tous les modèles Ollama
     * @param ollamaDir Répertoire Ollama
     * @return Liste des manifests
     */
    static std::vector<OllamaManifest> getOllamaManifests(const QString& ollamaDir);
    
    /**
     * @brief Remplit la liste des modèles Ollama
     * @param ollamaDir Répertoire Ollama
     * @param models Liste à remplir
     */
    static void getOllamaModels(const QString& ollamaDir, std::vector<LLMModel>& models);

    static const QString ollamaSystemDir;        ///< Répertoire système Ollama
    static const QString ollamaManifestBaseDir;  ///< Répertoire des manifests
    static const QString ollamaBlobsBaseDir;     ///< Répertoire des blobs

private:
    /**
     * @brief Envoie une requête interne
     * @param chat Chat associé
     * @param content Contenu à générer
     * @param streamed Indique si le streaming est activé
     */
    void postInternal(Chat* chat, const QString& content, bool streamed);
    
    /**
     * @brief Vérifie si le processus doit être démarré
     * @return true si le processus doit être démarré, false sinon
     */
    bool requireStartProcess();

    QString url_;                    ///< URL du serveur Ollama
    QString api_version_;            ///< Version de l'API
    QString api_generate_;           ///< Endpoint de génération
    QString apiKey_;                 ///< Clé API

    QString programPath_;            ///< Chemin vers l'exécutable Ollama
    QStringList programArguments_;   ///< Arguments pour l'exécutable
    std::shared_ptr<QProcess> programProcess_; ///< Processus Ollama

    QNetworkAccessManager* networkManager_; ///< Gestionnaire de réseau
};