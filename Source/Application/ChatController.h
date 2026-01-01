#pragma once

#include "LLMServices.h"
#include "RAGService.h"

class ChatController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Chat* currentChat READ currentChat NOTIFY currentChatChanged)
    Q_PROPERTY(QVariantList chatList READ chatList NOTIFY chatListChanged)
    Q_PROPERTY(int currentChatIndex READ currentChatIndex NOTIFY currentChatChanged)
    Q_PROPERTY(bool ragEnabled READ ragEnabled WRITE setRagEnabled NOTIFY ragEnabledChanged)
    Q_PROPERTY(RAGService* ragService READ ragService CONSTANT)

public:
    explicit ChatController(LLMServices* service, QObject* parent = nullptr);
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
    void chatContentUpdated(Chat* chat);
    void currentChatChanged();
    void chatListChanged();
    void availableModelsChanged();
    void loadingStarted();
    void loadingFinished();

private:
    void saveChats();
    void loadChats();
    
    QString getChatsFilePath() const;
    void notifyUpdatedChat(Chat* chat);
    void checkChatsProcessingFinished();
    void connectAPIsSignals();

    LLMServices* llmServices_;
    RAGService* ragService_;
    QList<Chat*> chats_;
    Chat* currentChat_;
    int chatCounter_;
    bool ragEnabled_ = false;

public:
    bool ragEnabled() const { return ragEnabled_; }
    void setRagEnabled(bool enabled)
    {
        if (ragEnabled_ != enabled)
        {
            ragEnabled_ = enabled;
            emit ragEnabledChanged();
        }
    }
    RAGService* ragService() const { return ragService_; }

signals:
    void ragEnabledChanged();
};
