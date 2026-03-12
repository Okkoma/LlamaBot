#include <QFile>
#include <QJsonDocument>
#include <QLibraryInfo>
#include <QStyleHints>
#include <QQuickStyle>
#include <QSettings>
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QApplication>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QPalette>
#include <QProcess>

#include "ThemeManager.h"

#ifdef Q_OS_ANDROID
QString DefaultStyle_ = "Material";
#else
QString DefaultStyle_ = "Basic";
#endif

QStringList quickStyles = 
{
    "Basic",
    "Fusion",
    "Imagine",
    "iOS",
    "macOS",
    "Material",
    "Universal",
    "Windows",
    "FluentWinUI3"
};

QStringList colorEmojiFonts = 
{
    "Noto Color Emoji", // Google, open source
    "Twemoji", // Twitter, open source        
    "Segoe UI Emoji", // Windows
    "Apple Color Emoji", // macOS/iOS
    "JoyPixels" // anciennement EmojiOne
};

bool isStyleAvailable(const QString &style)
{
    QString path = QLibraryInfo::path(QLibraryInfo::QmlImportsPath) + "/QtQuick/Controls/" + style;
    qDebug() << "isStyleAvailable:" << path << " :" << QDir(path).exists();
    return QDir(path).exists();
}

QStringList getPlatformStyles()
{
    QStringList styles;
    for (QString style : quickStyles) 
        if (isStyleAvailable(style))
            styles.push_back(style);
    return styles;
}

QString findColorEmojiFont()
{
    for (const QString &family : QFontDatabase::families())
        if (colorEmojiFonts.contains(family))
            return family;
    return {};
}


ThemeManager::ThemeManager(QObject* parent) : 
    QObject(parent)
{
    // Get default font from QApplication if available, otherwise use system default
    systemFont_ = qApp ? qApp->font().family() : QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    colorEmojiFont_ = findColorEmojiFont();

    loadSettings();

    styles_ = getPlatformStyles();
    if (!styles_.contains(currentStyle_))
    {
        // Set the fallback style
        setStyle(styles_.isEmpty() ? DefaultStyle_ : styles_.first());
        emit styleNotAvailableWarning(currentStyle_);
    }
    
    QQuickStyle::setStyle(currentStyle_);
    qDebug() << "ThemeManager: use style:" << currentStyle_;

    loadThemes();
}

void ThemeManager::restartApplication()
{
    QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    QCoreApplication::quit();
}

void ThemeManager::loadSettings()
{
    const QString defaultFont = !colorEmojiFont_.isEmpty() ? colorEmojiFont_ : systemFont_;
    const int defaultFontSize = 14;
    
    QSettings settings;
    setStyle(settings.value("ui/style", DefaultStyle_).toString());
    setTheme(settings.value("ui/theme", "Default").toString());
    setDarkMode(settings.value("ui/darkMode", false).toBool());
    setFont(settings.value("ui/fontFamily", defaultFont).toString());
    setFontSize(settings.value("ui/fontSize", defaultFontSize).toInt());
}

void ThemeManager::resetSettings()
{
    setTheme("Default");
    setDarkMode(false);
    setFont(!colorEmojiFont_.isEmpty() ? colorEmojiFont_ : QFontDatabase::systemFont(QFontDatabase::GeneralFont).family());
    setFontSize(14);
}

void ThemeManager::setStyle(const QString& style) 
{
    if (currentStyle_ != style)
    {
        currentStyle_ = style;
        QSettings().setValue("ui/style", currentStyle_);
    }
}

void ThemeManager::setTheme(const QString& theme)
{
    if (currentTheme_ != theme)
    {
        currentTheme_ = theme;
        QSettings().setValue("ui/theme", currentTheme_);  
        applyTheme();
    }
}

void ThemeManager::setFont(const QString& font)
{
    if (currentFont_ != font)
    {
        currentFont_ = font;
        QSettings().setValue("ui/fontFamily", currentFont_);
        emit fontChanged();
    }
}

void ThemeManager::setFontSize(int size)
{
    if (currentFontSize_ != size)
    {
        currentFontSize_ = size;
        QSettings().setValue("ui/fontSize", currentFontSize_);
        emit fontChanged();
    }
}

void ThemeManager::setDarkMode(bool dark)
{
    if (darkMode_ != dark)
    {
        darkMode_ = dark;
        QSettings().setValue("ui/darkMode", darkMode_);
        emit darkModeChanged();
        emit themeChanged();
    }
}

const QString& ThemeManager::currentFont() const
{
    return currentFont_;
}

int ThemeManager::currentFontSize() const
{
    return currentFontSize_;
}

const QString& ThemeManager::currentStyle() const 
{
    return currentStyle_;
}

const QString& ThemeManager::currentTheme() const
{
    return currentTheme_;
}

bool ThemeManager::darkMode() const
{ 
    return darkMode_; 
}

QStringList ThemeManager::availableStyles() const 
{
    return styles_;
}

QStringList ThemeManager::availableThemes() const
{
    return themes_;
}

QColor ThemeManager::color(const QString& elt) const
{
    return QColor(currentThemeData_[darkMode_ ? "dark" : "light"][elt].toString());
}

QColor ThemeManager::baseColor() const
{ 
    return color("base");
}

QColor ThemeManager::windowColor() const
{ 
    return color("window");
}

QColor ThemeManager::windowDarkerColor() const
{ 
    return color("windowDarker");
}

QColor ThemeManager::windowDarker2Color() const
{ 
    return color("windowDarker2");
}

QColor ThemeManager::windowTextColor() const
{ 
    return color("windowText");
}

QColor ThemeManager::accentColor() const
{ 
    return color("accent");
}

QColor ThemeManager::textColor() const
{ 
    return color("text");
}

QColor ThemeManager::buttonColor() const
{ 
    return color("button");
}

QColor ThemeManager::buttonTextColor() const
{ 
    return color("buttonText");
}

QColor ThemeManager::spacerColor() const
{
    return color("spacer");
}

void ThemeManager::loadThemes()
{
    // Chemin absolu vers le dossier des thèmes
    QString themesDirPath = QCoreApplication::applicationDirPath() + "/data/ressources/themes";
    QDir themesDir(themesDirPath);

    qDebug() << "Chemin des thèmes:" << themesDirPath;
    qDebug() << "Existe:" << themesDir.exists();

    if (!themesDir.exists())
    {
        // Le préfixe ":" indique à Qt de chercher dans les ressources compilées
        themesDirPath = ":/themes";
        themesDir.setPath(themesDirPath);
    }

    if (!themesDir.exists())
    {
        qWarning() << "Themes directory does not exist:" << themesDir.path();
        return;
    }
    
    // Lister les fichiers JSON dans le dossier
    QStringList themeFiles = themesDir.entryList(QStringList() << "*.json", QDir::Files);
    qDebug() << "Fichiers JSON trouvés:" << themeFiles;

    themes_.clear();
    for (const QString& fileName : themeFiles)
    {
        QFile file(themesDir.filePath(fileName));
        if (file.open(QIODevice::ReadOnly))
        {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject theme = doc.object();
            themes_.push_back(theme["name"].toString());
            dataThemes_[themes_.back()] = theme;
            
            qDebug() << "Thème chargé:" << themes_.back();
        }
    }

    qDebug() << "Nombre de thèmes chargés:" << themes_.size();

    if (!themes_.contains(currentTheme_))
        setTheme(themes_.front());
    else
        applyTheme();
}

void ThemeManager::applyTheme()
{
    currentThemeData_ = dataThemes_[currentTheme_];

    QPalette palette;
    palette.setColor(QPalette::Window, windowColor());
    palette.setColor(QPalette::WindowText, windowTextColor());
    palette.setColor(QPalette::Base, baseColor());
    palette.setColor(QPalette::Text, textColor());
    palette.setColor(QPalette::Button, buttonColor());
    palette.setColor(QPalette::ButtonText, buttonTextColor());
    QGuiApplication::setPalette(palette);

    // Set color scheme : not really useful because we're using darkMode property.
    QGuiApplication::styleHints()->setColorScheme(darkMode_ ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);

    qDebug() << "ThemeManager::applyTheme: theme:" << currentTheme_ << "style:" << QQuickStyle::name() << "dark:" << darkMode_;
    emit themeChanged();
}
