#pragma once

#include "Chat.h"

/**
 * @class ChatImpl
 * @brief Implémentation concrète de la classe Chat
 * 
 * Cette classe implémente l'interface abstraite Chat et fournit
 * une implémentation complète pour la gestion des conversations
 * avec les modèles LLM.
 */
class ChatImpl : public Chat
{
public:
    /**
     * @brief Factory de ChatImpl
     */
    static Chat* Create(LLMServices* service, const QString& name = "new_chat", const QString& initialContext = "",
                            bool streamed = true, QObject* parent = nullptr)
    {
        return new ChatImpl(service, name, initialContext, streamed, parent);
    }
    /**
     * @brief Constructeur de ChatImpl
     * @param service Services LLM à utiliser
     * @param name Nom du chat (par défaut: "new_chat")
     * @param initialContext Contexte initial pour le chat
     * @param streamed Indique si le streaming est activé (par défaut: true)
     * @param parent Objet parent Qt (optionnel)
     */
    explicit ChatImpl(LLMServices* service, const QString& name = "new_chat", const QString& initialContext = "",
        bool streamed = true, QObject* parent = nullptr);
    
    /**
     * @brief Définit l'API LLM à utiliser
     * @param api Nom de l'API à utiliser
     */
    void setApi(const QString& api) override;
    
    /**
     * @brief Définit le modèle LLM à utiliser
     * @param model Nom du modèle à utiliser
     */
    void setModel(const QString& model) override;

    /**
     * @brief Retourne l'historique des messages formaté
     * @return QString contenant l'historique des messages formaté
     */
    QString getFormattedHistory() const override;

    /**
     * @brief Retourne le premier message formaté correspondant au role à partir de l'index.
     * @return QString contenant le message formaté, ou QString vide si aucun message correspondant n'est trouvé
     * @see Chat.h     
     */
    QString getFormattedMessage(const QString& role, qsizetype index) const override;

    /**
     * @brief Met à jour le contenu du chat
     * @param content Nouveau contenu à ajouter au chat
     */
    void updateContent(const QString& content) override;
    
    /**
     * @brief Met à jour le flux courant de l'IA
     * @param text Texte à ajouter au flux courant de l'IA
     */
    void updateCurrentAIStream(const QString& text) override;

    // Serialization
    /**
     * @brief Sérialise le chat en JSON
     * @return Objet JSON représentant l'état complet du chat
     */
    QJsonObject toJson() const override;
    
    /**
     * @brief Désérialise le chat depuis JSON
     * @param json Objet JSON contenant les données du chat
     */
    void fromJson(const QJsonObject& json) override;

    // Export Helpers
    /**
     * @brief Retourne la conversation complète
     * @return Conversation complète sous forme de texte formaté
     */
    Q_INVOKABLE QString getFullConversation() const override;
    
    /**
     * @brief Retourne tous les prompts utilisateur
     * @return Tous les messages de l'utilisateur sous forme de texte
     */
    Q_INVOKABLE QString getUserPrompts() const override;
    
    /**
     * @brief Retourne toutes les réponses du bot
     * @return Toutes les réponses de l'IA sous forme de texte
     */
    Q_INVOKABLE QString getBotResponses() const override;

protected:
    /**
     * @brief Finalise le flux de streaming
     * 
     * Appelée lorsque le streaming se termine pour finaliser
     * le message courant de l'IA.
     */
    void finalizeStream() override;

private:
    /**
     * @brief Initialise le chat
     * 
     * Configure l'état initial du chat et les connexions.
     */
    void initialize();

    /**
     * @brief Ajoute du contenu à l'historique
     * @param role Rôle du message (user, assistant, system)
     * @param content Contenu du message
     */
    void addContent(const QString& role, const QString& content);

    void sanitizeStream(QString& text);

    int lastBotIndex_;       ///< Index du dernier message du bot

    QString userPrompt_;     ///< Prompt utilisateur courant
    QString aiPrompt_;       ///< Prompt de l'IA courant
    QString currentAIRole_{"assistant"};  ///< Rôle courant de l'IA (assistant, thought)
    QString currentAIStream_; ///< Flux courant de l'IA en cours de streaming
};
