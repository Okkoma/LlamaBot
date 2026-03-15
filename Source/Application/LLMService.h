#pragma once

#include "LLMServiceDefs.h"

class Chat;
class LLMServices;

class LLMService : public QObject
{
    Q_OBJECT

public:
    using LLMAPIFactory = std::function<LLMService*(LLMServices* llmservices, const QVariantMap& params)>;
    template <typename T>
    static void registerService(int type)
    {
        factories_[type] = [](LLMServices* llmservices, const QVariantMap& params)
        {
            return new T(llmservices, params);
        };
    }
    static LLMService* createService(LLMServices* llmservices, const QVariantMap& params)
    {
        if (!params.contains("type"))
            return nullptr;
        int type = params["type"].value<int>();
        auto it = factories_.find(type);
        return it != factories_.end() ? it->second(llmservices, params) : nullptr;
    }

    LLMService(int type, LLMServices* llmservices, const QString& name) :
        llmservices_(llmservices),
        type_(type),
        name_(name)
    { }

    LLMService(LLMServices* llmservices, const QVariantMap& params) :
        llmservices_(llmservices),
        type_(params["type"].toInt()),
        name_(params["name"].toString()),
        params_(params)
    {
        setApiKey(params["apikey"].toString());
    }

    virtual ~LLMService() { stop(); }

    void setState(int state) { state_ = state; }
    int getState() const { return state_; }

    virtual bool start() { return true; };
    virtual bool stop() { return true; };
    
    virtual bool isReady() const = 0;

    virtual void deleteModel(const LLMModel& model) {}

    void setApiKey(QString value);

    virtual void post(Chat* chat, const QString& content, bool streamed = true) {}

    virtual void stopStream(Chat* chat) { Q_UNUSED(chat); }

    virtual QString formatMessages(const Chat* chat) const { return {}; }
    
    virtual QString formatMessage(const Chat* chat, int historyIndex) const { return {}; }

    virtual bool handleMessageError(Chat* chat, const QString& message) { return false; }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["type"] = enumValueToString<LLMEnum>("LLMType", type_);
        obj["name"] = name_;
        obj["url"] = params_["url"].toString();
        obj["apiver"] = params_["apiver"].toString();
        obj["apigen"] = params_["apigen"].toString();
        obj["executable"] = params_["executable"].toString();
        obj["args"] = params_["programargs"].toJsonArray();
        return obj;
    }
    static LLMService* fromJson(LLMServices* llmservices, const QJsonObject& obj)
    {
        QVariantMap params;
        QString exe = obj["executable"].toString(); 
        if (exe != "" && !QFile::exists(exe))
        {
            qDebug() << "LLMService executable not find" << exe;
            return nullptr;
        }
        params["executable"] = exe;
        params["type"] = stringToEnumValue<LLMEnum>("LLMType", obj["type"].toString());
        params["name"] = obj["name"].toString();
        params["url"] = obj["url"].toString();
        params["apiver"] = obj["apiver"].toString();
        params["apigen"] = obj["apigen"].toString();
        QStringList programArguments;
        QJsonArray argsArray = obj["args"].toArray();
        for (const QJsonValue& val : argsArray)
            programArguments << val.toString();        
        params["programargs"] = programArguments;
        return LLMService::createService(llmservices, params);
    }

    virtual std::vector<float> getEmbedding(const QString& text) = 0;
    virtual const std::vector<LLMModel>& getAvailableModels() = 0;
    
    void markAvailableModelsDirty()
    {
        availableModelsDirty_ = true;
    }

    LLMServices* llmservices_;
    int type_;
    QString name_;
    QVariantMap params_;

signals:
    void modelLoadingStarted(const QString& modelName);
    void modelLoadingFinished(const QString& modelName, bool success);

    void stopStreamRequested();

protected:
    QString apiKey_;
    std::atomic<int> state_ {0};     ///< état interne du service
    bool availableModelsDirty_ { true };
    std::vector<LLMModel> availableModels_;

private:
    static std::unordered_map<int, LLMAPIFactory> factories_;
};
