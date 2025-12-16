#pragma once

#include "Chat.h"
#include "LLMService.h"
#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class ChatController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Chat* currentChat READ currentChat NOTIFY currentChatChanged)
    Q_PROPERTY(QVariantList chatList READ chatList NOTIFY chatListChanged)
    Q_PROPERTY(int currentChatIndex READ currentChatIndex NOTIFY currentChatChanged)

public:
    explicit ChatController(LLMService* service, QObject* parent = nullptr);
    ~ChatController();

    Chat* currentChat() const { return currentChat_; }
    QVariantList chatList() const;
    int currentChatIndex() const;

    // Chat Management
    Q_INVOKABLE void createChat();
    Q_INVOKABLE void switchToChat(int index);
    Q_INVOKABLE void deleteChat(int index);
    Q_INVOKABLE void renameChat(int index, const QString& name);

    // Message Operations
    Q_INVOKABLE void sendMessage(const QString& text);
    Q_INVOKABLE void stopGeneration();

    // Model Management
    Q_INVOKABLE QVariantList getAvailableModels();
    Q_INVOKABLE QVariantList getAvailableAPIs();
    Q_INVOKABLE void setModel(const QString& modelName);
    Q_INVOKABLE void setAPI(const QString& apiName);

signals:
    void currentChatChanged();
    void chatListChanged();
    void availableModelsChanged();
    void loadingStarted();
    void loadingFinished();

private:
    void checkChatsProcessingFinished();

    LLMService* service_;
    QList<Chat*> chats_;
    Chat* currentChat_;
    int chatCounter_;
};
