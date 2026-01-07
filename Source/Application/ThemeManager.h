#pragma once

#include <QObject>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QFont>
#include <qcolor.h>
#include <qobject.h>
#include <qtmetamacros.h>

/**
 * @class ThemeManager
 * @brief Gestionnaire des thèmes et styles de l'application
 * 
 * Cette classe gère les thèmes visuels, les styles et les préférences
 * d'affichage de l'application. Elle permet de changer dynamiquement
 * l'apparence de l'interface utilisateur.
 * 
 * Elle supporte les thèmes sombres et clairs, et permet de personnaliser
 * les couleurs et les polices.
 */
class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentStyle READ currentStyle WRITE setStyle NOTIFY styleChanged);
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setTheme NOTIFY themeChanged);
    Q_PROPERTY(QString currentFont READ currentFont WRITE setFont NOTIFY fontChanged);
    Q_PROPERTY(QString colorEmojiFont READ colorEmojiFont NOTIFY fontChanged);
    Q_PROPERTY(int currentFontSize READ currentFontSize WRITE setFontSize NOTIFY fontChanged);    
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)

public:
    /**
     * @brief Constructeur de ThemeManager
     * @param parent Objet parent Qt (optionnel)
     */
    explicit ThemeManager(QObject* parent = nullptr);
    
    /**
     * @brief Destructeur de ThemeManager
     * 
     * Nettoie les ressources et sauvegarde les paramètres.
     */
    ~ThemeManager() override;

    /**
     * @brief Définit le style courant
     * @param style Nom du style à appliquer
     */
    Q_INVOKABLE void setStyle(const QString& style);
    
    /**
     * @brief Définit le thème courant
     * @param theme Nom du thème à appliquer
     */
    Q_INVOKABLE void setTheme(const QString& theme);
    
    /**
     * @brief Définit la police courante
     * @param font Nom de la police à appliquer
     */
    Q_INVOKABLE void setFont(const QString& font);
    
    /**
     * @brief Définit la taille de la police courante
     * @param size Taille de la police à appliquer
     */
    Q_INVOKABLE void setFontSize(int size);
    
    /**
     * @brief Active ou désactive le mode sombre
     * @param dark true pour activer le mode sombre, false pour le désactiver
     */
    Q_INVOKABLE void setDarkMode(bool dark);
    
    /**
     * @brief Retourne le style courant
     * @return Nom du style courant
     */
    const QString& currentStyle() const;
    
    /**
     * @brief Retourne le thème courant
     * @return Nom du thème courant
     */
    const QString& currentTheme() const;
    
    /**
     * @brief Retourne la police courante
     * @return Nom de la police courante
     */
    const QString& currentFont() const;
    
    /**
     * @brief Retourne la police pour les emojis colorés
     * @return Nom de la police pour les emojis colorés
     */
    Q_INVOKABLE const QString& colorEmojiFont() const { return colorEmojiFont_; };
    
    /**
     * @brief Retourne la taille de la police courante
     * @return Taille de la police courante
     */
    int currentFontSize() const;
    
    /**
     * @brief Retourne si le mode sombre est activé
     * @return true si le mode sombre est activé, false sinon
     */
    bool darkMode() const;    
    
    /**
     * @brief Retourne la liste des styles disponibles
     * @return Liste des noms de styles disponibles
     */
    Q_INVOKABLE QStringList availableStyles() const;
    
    /**
     * @brief Retourne la liste des thèmes disponibles
     * @return Liste des noms de thèmes disponibles
     */
    Q_INVOKABLE QStringList availableThemes() const;
    
    /**
     * @brief Retourne la couleur pour un élément donné
     * @param elt Nom de l'élément
     * @return Couleur associée à l'élément
     */
    Q_INVOKABLE QColor color(const QString& elt);

    /**
     * @brief Charge les thèmes depuis les fichiers
     * 
     * Charge les définitions de thèmes à partir des fichiers JSON.
     */
    void loadThemes();
    
    /**
     * @brief Charge les paramètres depuis la configuration
     * 
     * Charge les préférences de thème sauvegardées.
     */
    void loadSettings();
    
    /**
     * @brief Sauvegarde les paramètres dans la configuration
     * 
     * Sauvegarde les préférences de thème courantes.
     */
    void saveSettings();

    /**
     * @brief Redémarre l'application
     */
    Q_INVOKABLE void restartApplication();

signals:
    /**
     * @brief Signal émis lorsque le style change
     */
    void styleChanged();
    
    /**
     * @brief Signal émis lorsque le thème change
     */
    void themeChanged();
    
    /**
     * @brief Signal émis lorsque le mode sombre change
     */
    void darkModeChanged();
    
    /**
     * @brief Signal émis lorsque la police change
     */
    void fontChanged();

    /**
     * @brief Signal émis lorsqu'un style n'est pas disponible
     * @param styleName Nom du style non disponible
     */
    void styleNotAvailableWarning(const QString& styleName);

private:
    /**
     * @brief Applique le thème courant
     * 
     * Met à jour l'interface utilisateur avec le thème sélectionné.
     */
    void applyTheme();

    bool darkMode_;                   ///< Indique si le mode sombre est activé
    QString currentStyle_;            ///< Style courant
    QString defaultStyle_;            ///< Style par défaut
    QStringList styles_;              ///< Liste des styles disponibles
    QStringList themes_;              ///< Liste des thèmes disponibles
    QString currentTheme_;            ///< Thème courant
    QJsonObject currentThemeData_;    ///< Données du thème courant
    QMap<QString, QJsonObject> dataThemes_; ///< Tous les thèmes chargés
    QString currentFont_;             ///< Police courante
    QString colorEmojiFont_;          ///< Police pour les emojis colorés
    int currentFontSize_;             ///< Taille de la police courante
};