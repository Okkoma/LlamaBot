#pragma once

#include "LLMServiceDefs.h"

class LLMServices;

struct ChatMessage
{
    QString role;
    QString content;
};

struct ChatData
{
    int n_ctx_{2048};
    int n_ctx_used_{0};
};

class Chat : public QObject
{
    Q_OBJECT
    Q_PROPERTY(const QString& currentApi READ getCurrentApi WRITE setApi NOTIFY currentApiChanged)
    Q_PROPERTY(const QString& currentModel READ getCurrentModel WRITE setModel NOTIFY currentModelChanged)
    Q_PROPERTY(const QStringList& messages READ getMessages NOTIFY messagesChanged)
    Q_PROPERTY(QVariantList history READ historyList NOTIFY historyChanged)
    Q_PROPERTY(int tokensConsumed READ getNumContextSizeUsed NOTIFY messagesChanged)
    Q_PROPERTY(int tokensTotal READ getNumContextSize NOTIFY messagesChanged)

public:
    Chat(LLMServices* llmservices, const QString& name = "new_chat", const QString& initialContext = "",
        bool streamed = true, QObject* parent = nullptr) :
        QObject(parent),
        llmservices_(llmservices),
        streamed_(streamed),
        processing_(false),
        name_(name),
        currentApi_("none"),
        currentModel_("none"),
        initialContext_(initialContext) {}
    virtual ~Chat() {}
    
    virtual void setApi(const QString& api) = 0;
    virtual void setModel(const QString& model) = 0;
    void setName(const QString& name) { name_ = name; }
    void setStreamed(bool enable) { streamed_ = enable; }
    void setProcessing(bool processing)
    {
        processing_ = processing;
        if (processing)
        {
            emit processingStarted(this);
        }
        else
        {
            if (streamed_)
                finalizeStream();
            emit processingFinished(this);
        }
    }

    virtual void updateContent(const QString& content) = 0;
    virtual void updateCurrentAIStream(const QString& text) = 0;

    void updateContextStates(int n_ctx, int n_ctx_used)
    { 
        data_.n_ctx_ = n_ctx; 
        data_.n_ctx_used_ = n_ctx_used;
    }

    const QString& getName() const { return name_; }
    bool getStreamed() const { return streamed_; }

    virtual QVariantList historyList() const = 0;
    const QString& getCurrentApi() const { return currentApi_; }
    const QString& getCurrentModel() const { return currentModel_; }
    const QStringList& getMessages() const { return messages_; }
    const QString& getMessagesRaw() const { return messagesRaw_; }
    QList<ChatMessage>& getHistory() { return history_; }
    QJsonObject& getInfo() { return info_; }
    bool isProcessing() const { return processing_; }
    
    int getNumContextSize() const { return data_.n_ctx_; };    
    int getNumContextSizeUsed() const { return data_.n_ctx_used_; };

    // Serialization
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject& json) = 0;

    // Export Helpers
    Q_INVOKABLE virtual QString getFullConversation() const = 0;
    Q_INVOKABLE virtual QString getUserPrompts() const = 0;
    Q_INVOKABLE virtual QString getBotResponses() const = 0;

signals:
    void currentApiChanged();
    void currentModelChanged();
    void messagesChanged();
    void streamUpdated(const QString& text); // For raw stream chunks if needed, or just rely on messagesChanged
    void inputCleared();                     // Request to clear input
    void processingStarted(Chat* chat);
    void processingFinished(Chat* Chat);
    void messageAdded(const QString& role, const QString& content);
    void streamFinishedSignal();
    void historyChanged();

protected:
    virtual void finalizeStream() = 0;

    // Data members
    ChatData data_;

    bool streamed_;
    bool processing_;

    QString name_;
    QString currentApi_;
    QString currentModel_;
    QString initialContext_;

    QStringList messages_;
    QString messagesRaw_;

    QList<ChatMessage> history_;
    QJsonObject info_;

    LLMServices* llmservices_;
};
