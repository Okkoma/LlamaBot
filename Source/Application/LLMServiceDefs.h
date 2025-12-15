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
    using LLMAPIFactory = std::function<LLMAPIEntry* (const QVariantMap& params)>;
    template<typename T> static void registerService(int type)
    {
        factories_[type] = [](const QVariantMap& params) { static T t = T(params); return &t; };
    }
    static LLMAPIEntry* createService(int type, const QVariantMap& params)
    {
        auto it = factories_.find(type);
        return it != factories_.end() ? it->second(params) : nullptr;
    }
    static LLMAPIEntry* fromJson(LLMService* service, const QJsonObject& obj);

    LLMAPIEntry(const QVariantMap& params);
    LLMAPIEntry(int type, LLMService* service, const QString& name);    
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

    LLMService* service_;
    int type_;
    QString name_; 

private:
    static std::unordered_map<int, LLMAPIFactory> factories_;
};
