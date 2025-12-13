#pragma once

#include "Chat.h"
#include "LLMService.h"
#include <QObject>

class ChatController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Chat* currentChat READ currentChat NOTIFY currentChatChanged)

public:
    explicit ChatController(LLMService* service, QObject* parent = nullptr) :
        QObject(parent), service_(service), currentChat_(nullptr)
    {
        createChat();
    }

    Chat* currentChat() const { return currentChat_; }

    Q_INVOKABLE void createChat()
    {
        Chat* chat = new Chat(service_, "New Chat", "", true, this);
        // Set pointers
        if (currentChat_ != chat)
        {
            currentChat_ = chat;
            emit currentChatChanged();
        }
    }

    Q_INVOKABLE void sendMessage(const QString& text)
    {
        if (!currentChat_)
            return;

        LLMAPIEntry* api = service_->get(currentChat_->currentApi());
        if (api)
        {
            // Trigger UI update immediately via chat
            // Chat::update(content) in sendRequest?
            // In ChatBotPanel, sendRequest calls llm->post which calls chat->updateContent.
            // But usually LLMService::post calls API which calls Chat?
            // Let's check LLMService::post.
            // LLMService::post -> api->post.
            // api->post usually calls chat->updateContent(content) (e.g. LlamaCppService::post).

            service_->post(api, currentChat_, text, true);
        }
    }

    Q_INVOKABLE void stopGeneration()
    {
        if (currentChat_)
            service_->stopStream(currentChat_);
    }

signals:
    void currentChatChanged();

private:
    LLMService* service_;
    Chat* currentChat_;
};
