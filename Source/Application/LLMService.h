#pragma once

#include "LLMServiceDefs.h"

class LLMService : public QObject
{
    Q_OBJECT

    friend class LLMAPIEntry;
    friend class OllamaService;

public:   
    explicit LLMService(QObject *parent);
    ~LLMService();

    void setWidget(QWidget* widget);
    void allowSharedModels(bool enable);

    LLMAPIEntry* get(const QString& name) const;

    const std::vector<LLMAPIEntry*>& getAPIs() const { return apiEntries_; }
    std::vector<LLMAPIEntry*> getAvailableAPIs() const;
    std::vector<LLMModel> getAvailableModels(const LLMAPIEntry* api=nullptr) const;

    void addAPI(LLMAPIEntry* info);

    void post(LLMAPIEntry* api, Chat* chat, const QString& content, bool streamed=true);
    void receive(LLMAPIEntry* api, Chat* chat, const QByteArray& data);
    void stopStream(Chat* chat);

private:
    void initialize();

    bool loadServiceJsonFile();
    bool saveServiceJsonFile();
    void createDefaultServiceJsonFile();

    bool requireStartProcess(LLMAPIEntry* api);

    void handleMessageError(Chat* chat, const QString& message);

    std::vector<LLMAPIEntry*> apiEntries_;

    QWidget* widget_;
    bool allowSharedModels_;

    QNetworkAccessManager* networkManager_;
};
