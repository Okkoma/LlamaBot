#pragma once

#include <QFutureWatcher>
#include <QObject>
#include <QQmlEngine>

#include "LLMService.h"
#include "VectorStore.h"

class LLMServices;

/**
 * @class RAGService
 * @brief Service de Retrieval-Augmented Generation (RAG)
 * 
 * Ce service implémente la fonctionnalité RAG qui permet d'augmenter
 * les capacités des modèles LLM en leur fournissant un contexte
 * pertinent extrait d'une base de connaissances.
 * 
 * Il gère l'ingestion de documents, la recherche de contexte,
 * et l'intégration avec les services LLM.
 */
class RAGService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString collectionStatus READ getCollectionStatus NOTIFY collectionStatusChanged)
    Q_PROPERTY(int topK READ getTopK WRITE setTopK NOTIFY topKChanged)
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnable NOTIFY enableChanged)

    QML_ELEMENT
    QML_UNCREATABLE("RAGService is provided by ApplicationServices")
    
public:
    /**
     * @brief Constructeur de RAGService
     * @param parent Objet parent Qt (optionnel)
     */
    explicit RAGService(QObject* parent = nullptr);
    
    /**
     * @brief Destructeur de RAGService
     * 
     * Nettoie les ressources et la base de connaissances.
     */
    ~RAGService();

    /**
     * @brief Active/désactive le service
     * @param enable état d'activation
     */    
    void setEnable(bool enable) { enabled_ = enable; }

    /**
     * @brief Retourne si RagService est activé
     * @return true si est activé, false sinon
     */
    bool isEnabled() const { return enabled_; }

    // Ingestion
    /**
     * @brief Ingère un fichier dans la base de connaissances
     * @param service LLMService à utiliser
     * @param filePath Chemin vers le fichier à ingérer
     * 
     * Traite le fichier et ajoute son contenu à la base vectorielle.
     */
    Q_INVOKABLE void ingestFile(LLMService* service, const QString& filePath);
    
    /**
     * @brief Ingère un répertoire dans la base de connaissances
     * @param service LLMService à utiliser
     * @param dirPath Chemin vers le répertoire à ingérer
     * 
     * Traite tous les fichiers du répertoire et les ajoute à la base vectorielle.
     */
    Q_INVOKABLE void ingestDirectory(LLMService* service, const QString& dirPath);
    
    /**
     * @brief Efface la collection actuelle
     * 
     * Supprime tous les documents de la base de connaissances.
     */
    Q_INVOKABLE void clearCollection();

    // Retrieval
    /**
     * @brief Récupère le contexte pour une requête
     * @param service LLMService à utiliser
     * @param query Requête de recherche
     * @return Contexte formaté pour le prompt
     * 
     * Recherche les documents pertinents et retourne un contexte
     * formaté pour être utilisé dans un prompt LLM.
     */
    Q_INVOKABLE QString retrieveContext(LLMService* service, const QString& query);

    // Search returning raw results (useful for UI showing sources)
    /**
     * @brief Effectue une recherche dans la base de connaissances
     * @param service LLMService à utiliser
     * @param query Requête de recherche
     * @return Liste des résultats de recherche
     * 
     * Effectue une recherche vectorielle et retourne les résultats bruts.
     */
    std::vector<SearchResult> search(LLMService* service, const QString& query);

    // Persistence
    /**
     * @brief Sauvegarde la collection sur disque
     * @return true si la sauvegarde a réussi, false sinon
     */
    Q_INVOKABLE bool saveCollection();
    
    /**
     * @brief Charge une collection depuis le disque
     * @return true si le chargement a réussi, false sinon
     */
    Q_INVOKABLE bool loadCollection();

    /**
     * @brief Retourne l'état de la collection
     * @return État actuel de la collection
     */
    QString getCollectionStatus() const;

    /**
     * @brief définit le nombre de resultats de recherche attendus
     *  @param topK Nombre de resultats de recherche attendus
    */
    void setTopK(int topK) { topK_ = topK; }

    /**
     * @brief Retourne le nombre de resultats de recherche attendus
     * @return Nombre de resultats attendus
     */    
    int getTopK() const { return topK_; }

    /**
     * @brief Retourne si RagService est vide
     * @return true si est vide, false sinon
     */
    bool isEmpty() const { return !vectorStore_.count(); }

    void resetSettings();

signals:
    /**
     * @brief Signal émis lorsque l'état de la collection change
     */
    void collectionStatusChanged();
    
    void topKChanged();

    void enableChanged();
    
    /**
     * @brief Signal émis lorsque l'ingestion est terminée
     * @param docsProcessed Nombre de documents traités
     * @param chunksAdded Nombre de chunks ajoutés
     */
    void ingestionFinished(int docsProcessed, int chunksAdded);
    
    /**
     * @brief Signal émis lorsqu'une erreur se produit
     * @param error Message d'erreur
     */
    void errorOccurred(const QString& error);

private:
    /**
     * @brief Traite un fichier en interne
     * @param service LLMService à utiliser
     * @param filePath Chemin vers le fichier à traiter
     * 
     * Méthode interne pour le traitement des fichiers.
     */
    void processFileInternal(LLMService* service, const QString& filePath);

    VectorStore vectorStore_;      ///< Base de données vectorielle
    QString status_;               ///< État actuel du service
    int topK_{5};                  ///< nombre de resultats de recherche
    bool enabled_{false};
    // In-memory embedding cache or similar could go here
    // For now simple direct calls
};