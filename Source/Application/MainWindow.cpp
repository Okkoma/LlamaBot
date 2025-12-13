#include <QAction>
#include <QFile>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFont>
#include <QDockWidget>
#include <QActionGroup>
#include <QQuickWidget>
#include <QQuickItem>
#include <QQmlContext>
#include <QQmlEngine>
#include <QDebug>

#include "ApplicationServices.h"

#include "ChatBotPanel.h"

#include "OllamaModelStoreDialog.h"


#include "MainWindow.h"


MainWindow::MainWindow(QWidget *parent, Qt::WindowFlags flags) :
	QMainWindow(parent, flags)
{
	setWindowTitle(QString{"ChatBot"});
	
    QString iconPath = "data/images/icon1.png";
    if (QFile::exists(iconPath))
        setWindowIcon(QIcon{iconPath});
    else
        qWarning() << "Icon file not found:" << iconPath;
       
    initialize();
}

MainWindow::~MainWindow()
{

}

void MainWindow::initialize()
{
    /// Create Menu
    
    // Menu Fichier
    QMenu* fileMenu = menuBar()->addMenu(tr("&Fichier"));

    // Action Quitter
    QAction* quitAction = new QAction(tr("&Quitter"), this);
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &MainWindow::Action_quitApplication);
    fileMenu->addAction(quitAction);

    // Menu Outils
    QMenu* toolsMenu = menuBar()->addMenu(tr("&Outils"));
    modelStoreAction_ = new QAction(tr("&Magasin de Modèles"), this);
    connect(modelStoreAction_, &QAction::triggered, this, &MainWindow::Action_openModelStore);
    toolsMenu->addAction(modelStoreAction_);

    // Menu Affichage
    QMenu* viewMenu = menuBar()->addMenu(tr("&Affichage"));
    
    // Actions Style
    materialDarkStyleAction_ = new QAction(tr("&Material Dark"), this);
    materialDarkStyleAction_->setCheckable(true);
    materialLightStyleAction_ = new QAction(tr("&Material Light"), this);
    materialLightStyleAction_->setCheckable(true);    
    QActionGroup* styleGroup = new QActionGroup(this);
    styleGroup->addAction(materialDarkStyleAction_);
    styleGroup->addAction(materialLightStyleAction_);
    materialDarkStyleAction_->setChecked(true);
    viewMenu->addSeparator();
    viewMenu->addAction(materialDarkStyleAction_);
    viewMenu->addAction(materialLightStyleAction_);
    connect(materialDarkStyleAction_, &QAction::triggered, this, &MainWindow::Action_setMaterialDarkStyle);
    connect(materialLightStyleAction_, &QAction::triggered, this, &MainWindow::Action_setMaterialLightStyle);

    /// Create Central Widget
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    chatBotWidget_ = new ChatBotPanel(ApplicationServices::get<LLMService>(), this);
    layout->addWidget(chatBotWidget_);
    centralWidget->setLayout(layout);
    setCentralWidget(centralWidget);
}

/// Setters

void MainWindow::setStyle(const QString& styleName)
{
    if (styleName == "Dark" || styleName == "Light")
    {
        emit styleChanged(styleName);
        materialDarkStyleAction_->setChecked(styleName == "Dark");
        materialLightStyleAction_->setChecked(styleName == "Light");
    } 
}

/// Actions

void MainWindow::Action_quitApplication()
{
    close();
}

void MainWindow::Action_setMaterialDarkStyle()
{
    qDebug() << "Action_setMaterialDarkStyle";
    setStyle("Dark");
}

void MainWindow::Action_setMaterialLightStyle()
{
    qDebug() << "Action_setMaterialLightStyle";
    setStyle("Light");
}

void MainWindow::Action_openModelStore()
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("Ollama Model Store");
    dialog->resize(600, 800);
    
    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QQuickWidget* quickWidget = new QQuickWidget(dialog);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    
    OllamaModelStoreDialog* client = new OllamaModelStoreDialog(dialog);
    client->setQmlEngine(quickWidget->engine());
    quickWidget->engine()->rootContext()->setContextProperty("ollamaModelStoreDialog", client);
    quickWidget->setSource(QUrl("qrc:/ressources/OllamaModelStoreDialog.qml"));
    
    // Connect QML close signal to Dialog close
    if (quickWidget->rootObject())
        connect(quickWidget->rootObject(), SIGNAL(closeRequested()), dialog, SLOT(accept()));
    
    layout->addWidget(quickWidget);
    dialog->exec();
    
    // Dialog and its children (client, widget) are deleted here automatically because 'dialog' is stack allocated? 
    // Wait, 'new QDialog(this)' is heap allocated but I didn't delete it in my previous thought.
    // Let's use deleteLater or manual delete.
    delete dialog;
}



