#include <QJsonDocument>
#include <QTabWidget>
#include <QTextEdit>
#include <QTextBlock>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QEvent>
#include <QIcon>
#include <QStyle>
#include <QProcess>
#include <QVector>
#include <QString>
#include <QMenu>
#include <QTabBar>
#include <QDir>
#include <QActionGroup>
#include <QScrollBar>
#include <qabstractslider.h>

#include <QDebug>

#include "LLMService.h"

#include "ChatBotPanel.h"


ChatBotPanel::ChatBotPanel(LLMService* service, QWidget* parent) :
    QWidget(parent),
    llm_(service)
{
    initialize();
    llm_->setWidget(this);
}

void ChatBotPanel::initialize()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Création du QTabWidget pour les chats
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabsClosable(true);
    mainLayout->addWidget(tabWidget_);

    // Installer un event filter sur la barre d'onglets pour le menu contextuel
    if (tabWidget_->tabBar())
        tabWidget_->tabBar()->installEventFilter(this);

    // Connexion pour fermer un onglet
    connect(tabWidget_, &QTabWidget::tabCloseRequested, this, [this](int index) {
        if (tabWidget_->count() > 1) 
        { // Toujours garder au moins un chat
            tabWidget_->removeTab(index);
            chats_.erase(chats_.begin() + index);
            if (index == tabWidget_->currentIndex())
                currentChat_ = &chats_[tabWidget_->currentIndex()];
        }
    });

    // Ajout d'un bouton "+" pour créer un nouvel onglet
    QPushButton* addTabButton = new QPushButton("+");
    addTabButton->setFixedWidth(30);
    tabWidget_->setCornerWidget(addTabButton, Qt::TopRightCorner);
    connect(addTabButton, &QPushButton::clicked, this, [this]() {
        currentChat_ = addNewChat();
    });

    setLayout(mainLayout);

    // Ajouter le premier chat
    currentChat_ = addNewChat();

    // Connexion du changement d'onglet pour mettre à jour le chat courant
    connect(tabWidget_, &QTabWidget::currentChanged, this, [this](int index) {
        currentChat_ = &chats_[index];
    });
}

Chat* ChatBotPanel::addNewChat()
{
    // Création d'un nouveau chat
    chats_.emplace_back(llm_);
    Chat* newChat = &chats_.back();

    // Création des widgets pour ce chat
    QWidget* chatTab = new QWidget;
    QVBoxLayout* tabLayout = new QVBoxLayout(chatTab);

    // Affichage de la conversation
    newChat->chatView_ = new QTextBrowser();
    newChat->chatView_->setReadOnly(true);
    tabLayout->addWidget(newChat->chatView_);

    // Zone de saisie + bouton
    QHBoxLayout* inputLayout = new QHBoxLayout();
    newChat->askText_ = new QTextEdit();
    newChat->askText_->setFixedHeight(50);
    QPushButton* sendButton = new QPushButton();
    sendButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    sendButton->setToolTip("Envoyer");
    inputLayout->addWidget(newChat->askText_);
    inputLayout->addWidget(sendButton);

    // Ajout du bouton Stop
    QPushButton* stopButton = new QPushButton();
    stopButton->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
    stopButton->setToolTip("Arrêter la génération");
    stopButton->setEnabled(false);
    inputLayout->addWidget(stopButton);
    newChat->stopButton_ = stopButton;
    tabLayout->addLayout(inputLayout);

    chatTab->setLayout(tabLayout);

    tabWidget_->addTab(chatTab, QString("Chat %1").arg(chats_.size()));
    tabWidget_->setCurrentWidget(chatTab);
    
    // Connexion du bouton et de la touche Entrée
    connect(sendButton, &QPushButton::clicked, this, [this, newChat]() {
        QString content = newChat->askText_->toPlainText().trimmed();
        if (!content.isEmpty()) {
            sendRequest(newChat, content);
            if (newChat->stopButton_)
                newChat->stopButton_->setEnabled(true);
        }
    });

    connect(stopButton, &QPushButton::clicked, this, [this, newChat, stopButton]() {
        llm_->stopStream(newChat);
        stopButton->setEnabled(false);
    });

    connect(newChat->chatView_->verticalScrollBar(), &QAbstractSlider::valueChanged, this, [this, newChat]() {
        newChat->lastScrollValue_ = newChat->chatView_->verticalScrollBar()->value();
    });

    newChat->askText_->installEventFilter(this);

    return newChat; 
}

void ChatBotPanel::sendRequest(Chat* chat, const QString& content, bool streamed)
{
    LLMAPIEntry* api = llm_->get(chat->currentApi_);
    if (api)
        llm_->post(api, chat, content, streamed);
    else
        qDebug() << "sendRequest: api" << chat->currentApi_ << "n'est pas disponible.";
}

// Ajout d'un eventFilter pour gérer Entrée
bool ChatBotPanel::eventFilter(QObject* obj, QEvent* event)
{
    // Gestion du menu contextuel sur la barre d'onglets
    if (obj == tabWidget_->tabBar() && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::RightButton)
        {
            QTabBar* tabBar = tabWidget_->tabBar();
            int tabIndex = tabBar->tabAt(mouseEvent->pos());
            Chat& chat = chats_[tabIndex];
            if (tabIndex != -1)
            {
                QMenu menu;
                // Label "Choisir API"
                QAction* apiLabel = menu.addAction("Choisir API");
                apiLabel->setEnabled(false);
                menu.addSeparator();
                // Liste des APIs disponibles
                QActionGroup* apiGroup = new QActionGroup(&menu);
                apiGroup->setExclusive(true);
                const std::vector<LLMAPIEntry*>& apis = llm_->getAPIs();
                for (LLMAPIEntry* api : apis)
                {
                    QAction* action = menu.addAction(api->name_);
                    action->setCheckable(true);
                    action->setChecked(chat.currentApi_ == api->name_);
                    apiGroup->addAction(action);
                }
                // Séparation
                menu.addSeparator();
                // Label "Choisir Modèle"
                QAction* modelLabel = menu.addAction("Choisir Modèle");
                modelLabel->setEnabled(false);
                menu.addSeparator();
                // Liste des modèles (scan dossier models/)
                QActionGroup* modelGroup = new QActionGroup(&menu);
                modelGroup->setExclusive(true);
                std::vector<LLMModel> models = llm_->getAvailableModels(llm_->get(chat.currentApi_));
                for (const LLMModel& model : models)
                {
                    QAction* action = menu.addAction(model.toString());
                    action->setCheckable(true);
                    action->setChecked(chat.currentModel_ == model.toString());
                    modelGroup->addAction(action);
                }
                QAction* selectedAction = menu.exec(tabBar->mapToGlobal(mouseEvent->pos()));
                // Gestion du choix API
                if (selectedAction && apiGroup->actions().contains(selectedAction))
                {
                    chat.setApi(selectedAction->text());
                }
                // Gestion du choix modèle
                if (selectedAction && modelGroup->actions().contains(selectedAction))
                {
                    chat.setModel(selectedAction->text());
                }

                return true;
            }
        }
    }
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (currentChat_ && keyEvent->key() == Qt::Key_Return && (keyEvent->modifiers() & Qt::ShiftModifier) == 0)
        {
            // Simule le clic sur le bouton Envoyer
            QString content = currentChat_->askText_->toPlainText().trimmed();
            if (!content.isEmpty())       
                sendRequest(currentChat_, content, true);                            
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}
