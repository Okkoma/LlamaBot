#pragma once

#include "Chat.h"

class ChatImpl : public Chat
{
public:
    explicit ChatImpl(LLMServices* service, const QString& name = "new_chat", const QString& initialContext = "",
        bool streamed = true, QObject* parent = nullptr);
    
    void setApi(const QString& api) override;
    void setModel(const QString& model) override;

    QVariantList historyList() const override;

    void updateContent(const QString& content) override;
    void updateCurrentAIStream(const QString& text) override;

    // Serialization
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject& json) override;

    // Export Helpers
    Q_INVOKABLE QString getFullConversation() const override;
    Q_INVOKABLE QString getUserPrompts() const override;
    Q_INVOKABLE QString getBotResponses() const override;

protected:
    void finalizeStream() override;

private:
    void initialize();

    void addContent(const QString& role, const QString& content);
    void addUserContent(const QString& text);
    void addAIContent(const QString& text);
    
    void updateObject();
   
    int lastBotIndex_;

    QString userPrompt_;
    QString aiPrompt_;
    QString currentAIStream_;     
};
