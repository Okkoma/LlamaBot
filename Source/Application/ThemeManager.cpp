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
    loadSettings();

    styles_ = getPlatformStyles();    

    if (!styles_.contains(currentStyle_))
    {
        // Set the fallback style
        currentStyle_ = styles_.isEmpty() ? "Basic" : styles_.first();    
        emit styleNotAvailableWarning(currentStyle_);
    }
    
    QQuickStyle::setStyle(currentStyle_);
    qDebug() << "ThemeManager: use style:" << currentStyle_;

    loadThemes();
}

ThemeManager::~ThemeManager()
{
    saveSettings();
}

void ThemeManager::restartApplication()
{
    saveSettings();
    QProcess::startDetached(QCoreApplication::applicationFilePath(), QCoreApplication::arguments());
    QCoreApplication::quit();
}

void ThemeManager::setStyle(const QString& style) 
{
    if (currentStyle_ != style)
    {
        currentStyle_ = style;
    }
}

void ThemeManager::setTheme(const QString& theme)
{
    if (currentTheme_ != theme)
    {
        currentTheme_ = theme;
        applyTheme();
    }
}

void ThemeManager::setFont(const QString& font)
{
    if (currentFont_ != font)
    {
        currentFont_ = font;
        emit fontChanged();
    }
}

void ThemeManager::setFontSize(int size)
{
    if (currentFontSize_ != size)
    {
        currentFontSize_ = size;
        emit fontChanged();
    }
}

void ThemeManager::setDarkMode(bool dark)
{
    if (darkMode_ != dark)
    {
        darkMode_ = dark;
        emit darkModeChanged();
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

QColor ThemeManager::color(const QString& elt)
{
    return QColor(currentThemeData_[darkMode_ ? "dark" : "light"][elt].toString());
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
    palette.setColor(QPalette::Window, color("window"));
    palette.setColor(QPalette::WindowText, color("windowText"));
    palette.setColor(QPalette::Base, color("base"));
    palette.setColor(QPalette::Text, color("text"));
    palette.setColor(QPalette::Button, color("button"));
    palette.setColor(QPalette::ButtonText, color("buttonText"));
    QGuiApplication::setPalette(palette);

    // Set color scheme : not really useful because we're using darkMode property.
    QGuiApplication::styleHints()->setColorScheme(darkMode_ ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);

    qDebug() << "ThemeManager::applyTheme: theme:" << currentTheme_ << "style:" << QQuickStyle::name() << "dark:" << darkMode_;
    emit themeChanged();
}

void ThemeManager::loadSettings()
{
    // Get default font from QApplication if available, otherwise use system default
    QFont systemFont = qApp ? qApp->font() : QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    colorEmojiFont_ = findColorEmojiFont();
    QString defaultFont = !colorEmojiFont_.isEmpty() ? colorEmojiFont_ : systemFont.family();
    int defaultFontSize = 14;

    QSettings settings;
    settings.beginGroup("ui");
    currentStyle_ = settings.value("style", defaultStyle_).toString();
    currentTheme_ = settings.value("theme", "").toString();
    darkMode_ = settings.value("darkMode", false).toBool();
    currentFont_ =  settings.value("fontFamily", defaultFont).toString();
    currentFontSize_ = settings.value("fontSize", defaultFontSize).toInt();
    settings.endGroup();

    qDebug() << "ThemeManager::loadSettings :"
        << "theme:" << currentTheme_
        << "style:" << currentStyle_
        << "dark:" << darkMode_        
        << "font:" << currentFont_
        << "fontsize:" << currentFontSize_;

    emit fontChanged();
}

void ThemeManager::saveSettings()
{
    QSettings settings;
    settings.beginGroup("ui");
    settings.setValue("style", currentStyle_);
    settings.setValue("theme", currentTheme_);    
    settings.setValue("darkMode", darkMode_);
    settings.setValue("fontFamily", currentFont_);
    settings.setValue("fontSize", currentFontSize_);
    settings.endGroup();

    qDebug() << "ThemeManager::saveSettings :"
        << "theme:" << currentTheme_
        << "style:" << currentStyle_
        << "dark:" << darkMode_        
        << "font:" << currentFont_
        << "fontsize:" << currentFontSize_;
}
