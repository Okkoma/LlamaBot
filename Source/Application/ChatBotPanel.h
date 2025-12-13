#pragma once

#include <QTextCursor>
#include <QVector>
#include <QWidget>

#include "Chat.h"

class QTabWidget;
class QTextBrowser;
class QTextEdit;
class QPushButton;

struct ChatViewData
{
    QTextBrowser* chatView;
    QTextEdit* askText;
    QPushButton* stopButton;
};

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

    std::vector<Chat*> chats_;
    QMap<Chat*, ChatViewData> chatViews_;
    Chat* currentChat_;
};
