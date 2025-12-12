#include <QCommandLineParser>
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QStyle>
#include <QDebug>
#include <QEvent>
#include <QKeyEvent>
#include <QRegularExpression>

#include "Application.h"
#include "LLMService.h"

#include "MainWindow.h"


Application::Application(int& argc, char **argv) :
	QApplication(argc, argv),
    services_(this)
{       
    setApplicationName("ChatBot");
    setApplicationVersion("0.0.1");
    
    QDir::setCurrent(applicationDirPath());
        
    QCommandLineParser parser;
    parser.setApplicationDescription("Description Application");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // Définition de l'option avec ses alias
    QCommandLineOption folderOption(
        QStringList() << "f" << "folder",  // -f OU --folder
        "Spécifie le dossier à utiliser",  // Description
        "dossier"                         // Nom du paramètre (pour l'aide)
    );
    parser.addOption(folderOption);
    
    // Parser les arguments
    parser.process(*this);

    // Vérification de l'option
    QString folderPath;
    if (parser.isSet(folderOption))
    {
        folderPath = parser.value(folderOption);
        qDebug() << "Dossier spécifié:" << folderPath;
    }
    else
    {
        folderPath = QDir::currentPath();
        qDebug() << "Dossier par défaut:" << folderPath;
    }

    window_ = new MainWindow();

    // Charger les paramètres utilisateur (style et fonte)
    loadSettings();
    setFont(currentFont_);

    connect(window_, &MainWindow::styleChanged, this, [this](const QString& style) {
        setStyleName(style);
        ApplyStyle(style);
        saveSettings();
    });

    window_->installEventFilter(this);
    
    window_->setStyle(currentStyleName_);
    window_->show();
}

Application::~Application()
{
    delete window_;
}

void Application::ApplyStyle(const QString& style)
{
    qDebug() << "Application: ApplyStyle:" << style;

    if (style == "Dark" || style == "Light")
    {    
        qApp->setStyle("Fusion");
        QFile file(":/ressources/material-" + style.toLower() + (style == "Light" ? "-new" : "") + ".qss");
        if (!file.exists())         
            qDebug() << "Application: ApplyStyle: can't find " << file.fileName();
        
        else if (file.open(QFile::ReadOnly))
        {
            QString styleSheet = QLatin1String(file.readAll());
            qApp->setStyleSheet(styleSheet);            
        }
    }

    setFont(currentFont_);
}

void Application::setFont(const QFont& font)
{
    qDebug() << "setFont: setFont:" << font;
    currentFont_ = font;
    qApp->setFont(font);
    // Ajouter la taille de fonte au style courant sans écraser les autres règles
    QString currentStyle = qApp->styleSheet();
    QString fontSizeRule = QString("QWidget { font-size: %1pt; }\n").arg(font.pointSize());
    // Supprimer toute ancienne règle de taille de fonte pour éviter les doublons
    QRegularExpression fontSizeRegex("QWidget \\{ font-size: [0-9]+pt; \\}\\n?");
    currentStyle.remove(fontSizeRegex);
    qApp->setStyleSheet(currentStyle + fontSizeRule);
    saveSettings();
}

void Application::setStyleName(const QString& styleName)
{
    currentStyleName_ = styleName;
    saveSettings();
}

QString Application::styleName() const
{
    return currentStyleName_;
}

void Application::saveSettings()
{
    QSettings settings;
    settings.beginGroup("ui");
    settings.setValue("fontFamily", currentFont_.family());
    settings.setValue("fontSize", currentFont_.pointSize());
    settings.setValue("style", currentStyleName_);
    settings.endGroup();
}

void Application::loadSettings()
{
    QSettings settings;
    settings.beginGroup("ui");
    QString fontFamily = settings.value("fontFamily", window_ ? window_->font().family() : "").toString();
    int fontSize = settings.value("fontSize", window_ ? window_->font().pointSize() : 10).toInt();
    currentStyleName_ = settings.value("style", "Fusion").toString();
    settings.endGroup();
    currentFont_ = QFont(fontFamily, fontSize);
}

bool Application::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        // Change the size of the font when "Ctrl"+"+" or "Ctrl"+"-"
        if ((keyEvent->modifiers() & Qt::ControlModifier) && (keyEvent->key() == Qt::Key_Plus || keyEvent->key() == Qt::Key_Minus))
        {
            setFont(QFont(currentFont_.defaultFamily(), currentFont_.pointSize()+ (keyEvent->key() == Qt::Key_Plus ? 1 : -1)));
            return true;
        }     
    }
    return false;
}
