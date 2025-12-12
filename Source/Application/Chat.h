#pragma once

#include <QString>
#include <QJsonObject>

#include "LLMService.h"


class QTextEdit;
class QTextBrowser;
class QPushButton;

struct Chat
{
    Chat(LLMService* service, const QString& name="new_chat", const QString& initialContext="", bool streamed=true);

    void initialize();
    void setApi(const QString& api);
    void setModel(const QString& model);
    void updateContent(const QString& content);

    void addContent(const QString& role, const QString& content);
    void finalizeStream();

    void addUserContent(const QString& text);
    void addAIContent(const QString& text);
    void updateCurrentAIStream(const QString& text);
    void updateObject();

    bool streamed_;
    int lastBotIndex_;
    int lastScrollValue_;
    
    QString name_;
    QString currentApi_;
    QString currentModel_;

    QString userPrompt_;
    QString aiPrompt_;

    QString initialContext_;
    QString rawMessages_;

    QStringList messages_;
    QString currentAIStream_;    
    QJsonObject jsonObject_;

    QTextBrowser* chatView_;
    QTextEdit* askText_;
    QPushButton* stopButton_ = nullptr;

    LLMService* service_;
};
