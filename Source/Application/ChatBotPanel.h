#pragma once

#include <QTextCursor>
#include <QVector>
#include <QWidget>

#include "Chat.h"

class QTabWidget;

class LLMService;

class ChatBotPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChatBotPanel(LLMService* llm, QWidget* parent);

    void sendRequest(Chat* chat, const QString& content, bool streamed = true);

    Chat* addNewChat();

private:
    void initialize();
    bool eventFilter(QObject* obj, QEvent* event) override;

    LLMService* llm_;

    QTabWidget* tabWidget_;

    std::vector<Chat> chats_;
    Chat* currentChat_;
};
