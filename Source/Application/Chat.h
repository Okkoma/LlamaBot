#pragma once

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>

#include "LLMService.h"

struct ChatMessage
{
    QString role;
    QString content;
};

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentApi READ currentApi WRITE setApi NOTIFY currentApiChanged)
    Q_PROPERTY(QString currentModel READ currentModel WRITE setModel NOTIFY currentModelChanged)
    Q_PROPERTY(QStringList messages READ messages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList history READ historyList NOTIFY historyChanged)

public:
    explicit Chat(LLMService* service, const QString& name = "new_chat", const QString& initialContext = "",
        bool streamed = true, QObject* parent = nullptr);

    void initialize();

    QVariantList historyList() const;

    QString currentApi() const { return currentApi_; }
    void setApi(const QString& api);

    QString currentModel() const { return currentModel_; }
    void setModel(const QString& model);

    QStringList messages() const { return messages_; }

    void updateContent(const QString& content);

    void addContent(const QString& role, const QString& content);
    void finalizeStream();

    void addUserContent(const QString& text);
    void addAIContent(const QString& text);
    void updateCurrentAIStream(const QString& text);
    void updateObject();

    void setProcessing(bool processing);

    bool isProcessing() const { return processing_; }

signals:
    void currentApiChanged();
    void currentModelChanged();
    void messagesChanged();
    void streamUpdated(const QString& text); // For raw stream chunks if needed, or just rely on messagesChanged
    void inputCleared();                     // Request to clear input
    void processingStarted();
    void processingFinished();
    void messageAdded(const QString& role, const QString& content);
    void streamFinishedSignal();
    void historyChanged();

public:
    // Data members
    bool streamed_;
    bool processing_;
    int lastBotIndex_;

    QString name_;
    QString currentApi_;
    QString currentModel_;

    QString userPrompt_;
    QString aiPrompt_;

    QString initialContext_;
    QString rawMessages_;

    QStringList messages_;
    QList<ChatMessage> history_;
    QString currentAIStream_;
    QJsonObject jsonObject_;

    LLMService* service_;
};
