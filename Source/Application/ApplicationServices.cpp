
#include <QDebug>

#include "ApplicationServices.h"

#include "LLMServices.h"
#include "LlamaCppService.h"
#include "OllamaService.h"


std::unordered_map<const char*, std::unique_ptr<QObject> > ApplicationServices::services_;

ApplicationServices::ApplicationServices(QObject* parent) :
    QObject(parent)
{
    // register llm api entries
    LLMService::registerService<LlamaCppService>(LLMEnum::LLMType::LlamaCpp);    
    LLMService::registerService<OllamaService>(LLMEnum::LLMType::Ollama);

    // add service
    services_.emplace(std::make_pair(LLMServices::staticMetaObject.className(), std::unique_ptr<LLMServices>(new LLMServices(this))));
    qDebug() << "ApplicationServices: add" << get<LLMServices>();
}

ApplicationServices::~ApplicationServices()
{
    services_.clear();
    qDebug() << "~ApplicationServices";
}