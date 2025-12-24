#pragma once

#include "LLMService.h"
#include "Chat.h"

class LLMServices : public QObject
{
    Q_OBJECT

    friend class RAGService;

public:
    explicit LLMServices(QObject* parent);
    ~LLMServices();

    void allowSharedModels(bool enable);
    void addAPI(LLMService* api);

    void stop(Chat* chat);
    void post(LLMService* api, Chat* chat, const QString& content, bool streamed = true);
    void receive(LLMService* api, Chat* chat, const QByteArray& data);

    bool hasSharedModels() const { return allowSharedModels_; }
    bool isServiceAvailable(LLMEnum::LLMType service) const;
    LLMService* get(LLMEnum::LLMType service) const;
    LLMService* get(const QString& name) const;
    const std::vector<LLMService*>& getAPIs() const;
    std::vector<LLMService*> getAvailableAPIs() const;  
    std::vector<LLMModel> getAvailableModels(const LLMService* api = nullptr);
    LLMModel getModel(const QString& name) const;
    std::vector<float> getEmbedding(const QString& text);

    bool loadServiceJsonFile();
    bool saveServiceJsonFile();

private:
    void initialize();
    void createDefaultServiceJsonFile();

    bool requireStartProcess(LLMService* api);
    void handleMessageError(Chat* chat, const QString& message);

    std::vector<LLMService*> apiEntries_;
    bool allowSharedModels_;    
};
