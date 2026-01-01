#pragma once

#include <QFutureWatcher>
#include <QObject>

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

public:
    /**
     * @brief Constructeur de RAGService
     * @param LLMServices Services LLM à utiliser
     * @param parent Objet parent Qt (optionnel)
     */
    explicit RAGService(LLMServices* LLMServices, QObject* parent = nullptr);
    
    /**
     * @brief Destructeur de RAGService
     * 
     * Nettoie les ressources et la base de connaissances.
     */
    ~RAGService();

    // Ingestion
    /**
     * @brief Ingère un fichier dans la base de connaissances
     * @param filePath Chemin vers le fichier à ingérer
     * 
     * Traite le fichier et ajoute son contenu à la base vectorielle.
     */
    Q_INVOKABLE void ingestFile(const QString& filePath);
    
    /**
     * @brief Ingère un répertoire dans la base de connaissances
     * @param dirPath Chemin vers le répertoire à ingérer
     * 
     * Traite tous les fichiers du répertoire et les ajoute à la base vectorielle.
     */
    Q_INVOKABLE void ingestDirectory(const QString& dirPath);
    
    /**
     * @brief Efface la collection actuelle
     * 
     * Supprime tous les documents de la base de connaissances.
     */
    Q_INVOKABLE void clearCollection();

    // Retrieval
    /**
     * @brief Récupère le contexte pour une requête
     * @param query Requête de recherche
     * @param topK Nombre de résultats à retourner (par défaut: 3)
     * @return Contexte formaté pour le prompt
     * 
     * Recherche les documents pertinents et retourne un contexte
     * formaté pour être utilisé dans un prompt LLM.
     */
    Q_INVOKABLE QString retrieveContext(const QString& query, int topK = 3);

    // Search returning raw results (useful for UI showing sources)
    /**
     * @brief Effectue une recherche dans la base de connaissances
     * @param query Requête de recherche
     * @param topK Nombre de résultats à retourner (par défaut: 3)
     * @return Liste des résultats de recherche
     * 
     * Effectue une recherche vectorielle et retourne les résultats bruts.
     */
    std::vector<SearchResult> search(const QString& query, int topK = 3);

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

signals:
    /**
     * @brief Signal émis lorsque l'état de la collection change
     */
    void collectionStatusChanged();
    
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
     * @param filePath Chemin vers le fichier à traiter
     * 
     * Méthode interne pour le traitement des fichiers.
     */
    void processFileInternal(const QString& filePath);

    LLMServices* llmServices_;      ///< Services LLM utilisés
    VectorStore vectorStore_;      ///< Base de données vectorielle
    QString status_;               ///< État actuel du service

    // In-memory embedding cache or similar could go here
    // For now simple direct calls
};