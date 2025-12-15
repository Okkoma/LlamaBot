
#include <QDebug>

#include "ApplicationServices.h"

#include "LLMService.h"
#include "LlamaCppService.h"
#include "OllamaService.h"


std::unordered_map<const char*, std::unique_ptr<QObject> > ApplicationServices::services_;

ApplicationServices::ApplicationServices(QObject* parent) :
    QObject(parent)
{
    // register llm api entries
    LLMAPIEntry::registerService<LlamaCppService>(LLMEnum::LLMType::LlamaCpp);    
    LLMAPIEntry::registerService<OllamaService>(LLMEnum::LLMType::Ollama);

    // add service
    services_.emplace(std::make_pair(LLMService::staticMetaObject.className(), std::unique_ptr<LLMService>(new LLMService(this))));
    qDebug() << "ApplicationServices: add" << get<LLMService>();
}

ApplicationServices::~ApplicationServices()
{
    services_.clear();
    qDebug() << "~ApplicationServices";
}