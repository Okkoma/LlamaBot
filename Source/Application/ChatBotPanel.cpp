#include <QActionGroup>
#include <QDir>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonDocument>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QString>
#include <QStyle>
#include <QTabBar>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector>
#include <qabstractslider.h>

#include <QDebug>

#include "LLMService.h"

#include "ChatBotPanel.h"

ChatBotPanel::ChatBotPanel(LLMService* service, QWidget* parent) : QWidget(parent), llm_(service)
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
    connect(tabWidget_, &QTabWidget::tabCloseRequested, this,
        [this](int index)
        {
            if (tabWidget_->count() > 1)
            { // Toujours garder au moins un chat
                Chat* chatToRemove = chats_[index];

                tabWidget_->removeTab(index);

                // Cleanup
                chatViews_.remove(chatToRemove);
                chats_.erase(chats_.begin() + index);

                chatToRemove->deleteLater();

                if (index < chats_.size())
                    currentChat_ = chats_[index];
                else if (!chats_.empty())
                    currentChat_ = chats_.back();
            }
        });

    // Ajout d'un bouton "+" pour créer un nouvel onglet
    QPushButton* addTabButton = new QPushButton("+");
    addTabButton->setFixedWidth(30);
    tabWidget_->setCornerWidget(addTabButton, Qt::TopRightCorner);
    connect(addTabButton, &QPushButton::clicked, this,
        [this]()
        {
            currentChat_ = addNewChat();
        });

    setLayout(mainLayout);

    // Ajouter le premier chat
    currentChat_ = addNewChat();

    // Connexion du changement d'onglet pour mettre à jour le chat courant
    connect(tabWidget_, &QTabWidget::currentChanged, this,
        [this](int index)
        {
            if (index >= 0 && index < static_cast<int>(chats_.size()))
                currentChat_ = chats_[index];
        });
}

Chat* ChatBotPanel::addNewChat()
{
    // Création d'un nouveau chat
    // Passage de this comme parent
    Chat* newChat = new Chat(llm_, "new_chat", "", true, this);
    chats_.push_back(newChat);

    // Création des widgets pour ce chat
    QWidget* chatTab = new QWidget;
    QVBoxLayout* tabLayout = new QVBoxLayout(chatTab);

    // Affichage de la conversation
    QTextBrowser* chatView = new QTextBrowser();
    chatView->setReadOnly(true);
    tabLayout->addWidget(chatView);

    // Zone de saisie + bouton
    QHBoxLayout* inputLayout = new QHBoxLayout();
    QTextEdit* askText = new QTextEdit();
    askText->setFixedHeight(50);
    QPushButton* sendButton = new QPushButton();
    sendButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    sendButton->setToolTip("Envoyer");
    inputLayout->addWidget(askText);
    inputLayout->addWidget(sendButton);

    // Ajout du bouton Stop
    QPushButton* stopButton = new QPushButton();
    stopButton->setIcon(style()->standardIcon(QStyle::SP_BrowserStop));
    stopButton->setToolTip("Arrêter la génération");
    stopButton->setEnabled(false);
    inputLayout->addWidget(stopButton);

    // Store View Data
    ChatViewData viewData = { chatView, askText, stopButton };
    chatViews_[newChat] = viewData;

    tabLayout->addLayout(inputLayout);
    chatTab->setLayout(tabLayout);

    tabWidget_->addTab(chatTab, QString("Chat %1").arg(chats_.size()));
    tabWidget_->setCurrentWidget(chatTab);

    // Connexion du bouton et de la touche Entrée
    connect(sendButton, &QPushButton::clicked, this,
        [this, newChat, askText]()
        {
            QString content = askText->toPlainText().trimmed();
            if (!content.isEmpty())
            {
                sendRequest(newChat, content);
            }
        });

    connect(stopButton, &QPushButton::clicked, this,
        [this, newChat, stopButton]()
        {
            llm_->stopStream(newChat);
            stopButton->setEnabled(false);
        });

    // Chat Logic CONNECTIONS

    // 1. Messages Changed
    connect(newChat, &Chat::messagesChanged, this,
        [newChat, chatView]()
        {
            chatView->setMarkdown(newChat->messages().join("\n\n"));
        });

    // 2. Stream Updated (Smart Scroll)
    connect(newChat, &Chat::streamUpdated, this,
        [newChat, chatView]()
        {
            QScrollBar* sb = chatView->verticalScrollBar();
            int scrollValue = sb->value();
            // If we are near the bottom (within 20px), auto-scroll
            bool atBottom = scrollValue >= sb->maximum() - 20;
            if (atBottom)
            {
                chatView->moveCursor(QTextCursor::End);
                chatView->ensureCursorVisible();
            }
            else
            {
                // Keep position
                // Changing markdown might change range, but usually we want to stay put if user scrolled up
            }
        });

    // 3. Input Handling
    connect(newChat, &Chat::inputCleared, this,
        [askText]()
        {
            askText->clear();
        });

    // 4. Processing State
    connect(newChat, &Chat::processingStarted, this,
        [stopButton]()
        {
            stopButton->setEnabled(true);
        });

    connect(newChat, &Chat::processingFinished, this,
        [stopButton]()
        {
            stopButton->setEnabled(false);
        });

    askText->installEventFilter(this);

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
            if (tabIndex == -1)
                return false;

            Chat* chat = chats_[tabIndex];
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
                action->setChecked(chat->currentApi_ == api->name_);
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
            std::vector<LLMModel> models = llm_->getAvailableModels(llm_->get(chat->currentApi_));
            for (const LLMModel& model : models)
            {
                QAction* action = menu.addAction(model.toString());
                action->setCheckable(true);
                action->setChecked(chat->currentModel_ == model.toString());
                modelGroup->addAction(action);
            }
            QAction* selectedAction = menu.exec(tabBar->mapToGlobal(mouseEvent->pos()));
            // Gestion du choix API
            if (selectedAction && apiGroup->actions().contains(selectedAction))
            {
                qDebug() << "selectedAction: api=" << selectedAction->text();
                chat->setApi(selectedAction->text());
            }
            // Gestion du choix modèle
            if (selectedAction && modelGroup->actions().contains(selectedAction))
            {
                qDebug() << "selectedAction: model=" << selectedAction->text();
                chat->setModel(selectedAction->text());
            }

            return true;
        }
    }
    if (event->type() == QEvent::KeyPress && currentChat_)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Find which chat corresponds to the object if possible, or use currentChat_
        // But obj is askText.

        ChatViewData& view = chatViews_[currentChat_];
        if (obj == view.askText)
        {
            if (keyEvent->key() == Qt::Key_Return && (keyEvent->modifiers() & Qt::ShiftModifier) == 0)
            {
                // Simule le clic sur le bouton Envoyer
                QString content = view.askText->toPlainText().trimmed();
                if (!content.isEmpty())
                    sendRequest(currentChat_, content, true);
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
