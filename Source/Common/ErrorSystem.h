#pragma once

#include <QString>
#include <QMap>
#include <QMutex>
#include <QDateTime>
#include <QList>

/**
 * @class ErrorSystem ThreadSafe
 * @brief Système centralisé de gestion et de journalisation des erreurs.
 *
 * Cette classe fournit un point d'accès unique (singleton) pour :
 * - Enregistrer des messages d'erreur associés à un code
 * - Logger des erreurs survenues à l'exécution avec des paramètres optionnels
 * - Consulter le dernier message d'erreur ou l'historique complet
 *
 */
class ErrorSystem
{
public:
    /**
     * @brief Retourne l'instance unique du système d'erreurs.
     *
     * Fournit l'accès singleton au système de gestion des erreurs.
     */
    static ErrorSystem& instance();

    /**
     * @brief Constructeur de copie supprimé.
     *
     * Empêche la copie de l'instance singleton.
     */
    ErrorSystem(const ErrorSystem&) = delete;

    /**
     * @brief Opérateur d'affectation supprimé.
     *
     * Empêche l'affectation de l'instance singleton.
     */
    ErrorSystem& operator=(const ErrorSystem&) = delete;

    /**
     * @brief Enregistre un message d'erreur.
     *
     * @param message Message d'erreur lisible associé à ce code.
     *
     * @return le code d'erreur.     
     */
    int registerError(const QString& message);

    /**
     * @brief Journalise une erreur survenue à l'exécution.
     *
     * @param code   Code d'erreur précédemment enregistré via @ref registerErrorMessage.
     * @param params Liste optionnelle de paramètres venant remplacer les placeholders
     *               présents dans le message d'erreur associé au code.
     */
    void logError(int code, const QStringList& params = QStringList());

    /**
     * @brief Retourne le/les message(s) d'erreur formaté(s) à partir de l'historique.
     *
     * Chaque message retourné contient :
     * - La date et l'heure d'enregistrement de l'erreur
     * - Le texte du message d'erreur associé au code
     * - Les paramètres éventuellement fournis lors de l'appel à @ref logError,
     *   appliqués sur les placeholders (par ex. %1, %2, ...) du message.
     *
     * @param index Index du premier message dans l'historique :
     *              - Si @p index >= 0, l'indexation se fait depuis le plus ancien
     *                message (0 = premier message enregistré).
     *              - Si @p index < 0, l'indexation se fait depuis le plus récent
     *                message (-1 = dernier message, -2 = avant-dernier, etc.).
     *              - Si @p index est hors bornes après normalisation, il est clampé
     *                pour garantir qu'au moins un message soit retourné.
     * @param count Nombre maximum de messages d'erreur à retourner :
     *              - Si @p count <= 0 (valeur par défaut -1), tous les messages
     *                disponibles à partir de @p index jusqu'au plus récent sont retournés.
     *              - Si @p count > 0, au plus @p count messages sont retournés,
     *                sans dépasser la fin de l'historique.
     *
     * @return La liste des messages d'erreur formatés dans l'ordre du plus ancien au plus récent
     *         Cette liste est vide si aucun historique (jamais vide si l'historique
     *         contient au moins un message).
     */
    QStringList getErrors(int index=0, int count=-1) const;

    void clearHistory();

    qsizetype getNumTypes() const { return errorTypes_.size(); }
    qsizetype size() const { return loggedErrors_.size(); }

private:
    /**
     * @brief Constructeur privé du système d'erreurs.
     *
     * Utilisé uniquement en interne pour implémenter le pattern singleton.
     */
    ErrorSystem() = default;

    struct ErrorInfo {
        ErrorInfo(int code, const QDateTime& ts, const QStringList& params) :
            error_(code),
            timestamp_(ts),
            params_(params) {}
        int error_;            ///< code d'erreur.
        QDateTime timestamp_;  ///< Date et heure auxquelles l'erreur a été enregistrée.
        QStringList params_;   ///< Paramètres utilisés pour formatter le message.
    };

    /**
     * @brief Formate un enregistrement d'erreur en chaîne prête à l'affichage.
     *
     * Applique les paramètres sur les placeholders du message, puis ajoute
     * le timestamp en préfixe.
     */
    QString formatError(const ErrorInfo& info) const;

    QList<QString> errorTypes_;         ///< Table des types d'erreur enregistrés, le code d'erreur est l'index dans la liste.
    QList<ErrorInfo> loggedErrors_;     ///< Historique des erreurs journalisées.
    mutable QMutex mutex_;              ///< Mutex protégeant l'accès concurrent aux données internes.
};

