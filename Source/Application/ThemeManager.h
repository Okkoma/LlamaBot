#pragma once

#include <QObject>
#include <QJsonObject>
#include <QMap>
#include <QList>
#include <QFont>
#include <qcolor.h>
#include <qobject.h>
#include <qtmetamacros.h>

class ThemeManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentStyle READ currentStyle WRITE setStyle NOTIFY styleChanged);
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setTheme NOTIFY themeChanged);
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)

public:
    explicit ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() override;

    Q_INVOKABLE void setStyle(const QString& style);
    Q_INVOKABLE void setTheme(const QString& theme);
    Q_INVOKABLE void setDarkMode(bool dark);
    void setFont(const QFont& font);

    const QString& currentStyle() const;
    const QString& currentTheme() const;
    bool darkMode() const;    
    Q_INVOKABLE QStringList availableStyles() const;
    Q_INVOKABLE QStringList availableThemes() const;
    Q_INVOKABLE QColor color(const QString& elt);

    void loadThemes();
    void loadSettings();
    void saveSettings();

signals:
    void styleChanged();
    void themeChanged();
    void darkModeChanged();
    void styleNotAvailableWarning(const QString& styleName);

private:
    void applyTheme();

    bool darkMode_;
    QString currentStyle_;
    QString defaultStyle_;
    QStringList styles_;
    QStringList themes_;
    QString currentTheme_;
    QJsonObject currentThemeData_;
    QMap<QString, QJsonObject> dataThemes_;
    QFont currentFont_;
};