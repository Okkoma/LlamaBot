#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>

#include <QDirIterator>
#include <QStandardPaths>

#include "LLMService.h"


std::unordered_map<int, LLMAPIEntry::LLMAPIFactory> LLMAPIEntry::factories_;


LLMAPIEntry::LLMAPIEntry(int type, LLMService* service, const QString& name) :
    service_(service),
    type_(type),
    name_(name)
{ }

LLMAPIEntry::LLMAPIEntry(const QVariantMap& params) :
    service_(params["lmservice"].value<LLMService*>()),
    type_(params["type"].toInt()),
    name_(params["name"].toString())
{ }

LLMAPIEntry::~LLMAPIEntry()
{
    bool ok = stop();
}

LLMAPIEntry* LLMAPIEntry::fromJson(LLMService* service, const QJsonObject& obj)
{
    int type = stringToEnumValue<LLMEnum>("LLMType", obj["type"].toString());

    QVariantMap params;
    params["lmservice"] = QVariant::fromValue(service);
    params["type"] = type;
    params["name"] = obj["name"].toString();

    if (type == LLMEnum::LLMType::Ollama)
    {
        params["executable"] = obj["executable"].toString();
        if (!QFile::exists(params["executable"].value<QString>()))
        {
            qDebug() << "Ollama not find" << params["executable"].value<QString>();
            return nullptr;
        }

        params["url"] = obj["url"].toString();
        params["apiver"] = obj["apiver"].toString();
        params["apigen"] = obj["apigen"].toString();
        params["apikey"] = obj["apikey"].toString();
        
        QStringList programArguments;
        QJsonArray argsArray = obj["args"].toArray();
        for (const QJsonValue& val : argsArray)
            programArguments << val.toString();        
        params["programargs"] = programArguments;
    }
    
    return createService(type, params);   
}
