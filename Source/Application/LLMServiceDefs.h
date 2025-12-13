#pragma once

#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QThread>
#include <QUrl>

#include "define.h"

class QNetworkAccessManager;
class QNetworkReply;

struct Chat;

class LLMEnum : public QObject
{
    Q_OBJECT

public:
    enum LLMType
    {
        LlamaCpp,
        Ollama,
        OpenAI,
    };
    Q_ENUM(LLMType)
};

class LLMService;

class LLMModel
{
public:
    LLMModel() = default;

    QString toString() const { return name_ + ":" + num_params_; }

    QString name_;
    QString format_;
    QString num_params_;
    QString vendor_;
    QString filePath_;
};

class LLMAPIEntry
{
public:
    LLMAPIEntry(LLMService* service, const QString& name, int type);
    virtual ~LLMAPIEntry();

    virtual bool start() { return true; };
    virtual bool stop() { return true; };
    virtual void setModel(Chat* chat, QString model = "") {}
    virtual bool isReady() const = 0;

    virtual void post(Chat* chat, const QString& content, bool streamed = true) = 0;
    virtual QString formatMessage(Chat* chat, const QString& role, const QString& content) { return content; }
    virtual void stopStream(Chat* chat) { Q_UNUSED(chat); }

    virtual bool handleMessageError(Chat* chat, const QString& message) { return false; }

    virtual QJsonObject toJson() const = 0;

    virtual std::vector<LLMModel> getAvailableModels() const = 0;

    int type_;
    QString name_;
    LLMService* service_;

    static LLMAPIEntry* fromJson(LLMService* service, const QJsonObject& obj);
};
