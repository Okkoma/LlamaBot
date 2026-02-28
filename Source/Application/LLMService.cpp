#include "LLMService.h"

std::unordered_map<int, LLMService::LLMAPIFactory> LLMService::factories_;

void LLMService::setApiKey(QString value)
{
    QSettings settings;
    QString apiKeyName = name_ + "ApiKey";
    if (value.isEmpty())
    {
        // try to get the key from qSettings
        value = settings.value(apiKeyName).toString();
    }
    if (!value.isEmpty())
    {
        apiKey_ = value;
        params_["apikey"] = value;
        settings.setValue(apiKeyName, apiKey_);
        qDebug() << "setApiKey: " << apiKeyName;
    }
}