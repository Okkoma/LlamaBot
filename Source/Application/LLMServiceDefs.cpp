#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>

#include "LLMService.h"
#include "LlamaCppService.h"
#include "OllamaService.h"


LLMAPIEntry::LLMAPIEntry(LLMService* service, const QString& name, int type) :
    type_(type),
    name_(name),
    service_(service)
{ }

LLMAPIEntry::~LLMAPIEntry()
{
    bool ok = stop();
}

std::vector<LLMModel> LLMAPIEntry::getAvailableModels() const
{
    std::vector<LLMModel> result;

    if (type_ == LLMEnum::LLMType::Ollama || service_->allowSharedModels_)
    {
        OllamaService::getOllamaModels(OllamaService::ollamaSystemDir, result);
        OllamaService::getOllamaModels(QDir::homePath() + "/", result);
    }

    return result;
}

LLMAPIEntry* LLMAPIEntry::fromJson(LLMService* service, const QJsonObject &obj)
{
    int type = stringToEnumValue<LLMEnum>("LLMType", obj["type"].toString());

    if (type == LLMEnum::LLMType::LlamaCpp)
    {
        LlamaCppService* llamaApi = LlamaCppService::createDefault(service, obj["name"].toString());

        return llamaApi;
    }
    else if (type == LLMEnum::LLMType::Ollama)
    {
        QStringList programArguments;
        QJsonArray argsArray = obj["args"].toArray();
        for (const QJsonValue &val : argsArray)
            programArguments << val.toString();

        return new OllamaService(service, 
            obj["name"].toString(),
            obj["url"].toString(),
            obj["apiver"].toString(),
            obj["apigen"].toString(),
            obj["apikey"].toString(),
            obj["executable"].toString(),
            programArguments
        );
    }
    return nullptr;
}
