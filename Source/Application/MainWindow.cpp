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

#include <QDebug>

#include "ApplicationServices.h"

#include "ChatBotPanel.h"

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



