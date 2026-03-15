#include <QDebug>

#include "LLMServices.h"
#include "LlamaCppService.h"
#include "OllamaService.h"
#include "RAGService.h"

#include "ModelSource.h"
#include "OllamaModelSource.h"
#include "HuggingFaceModelSource.h"

#include "ApplicationServices.h"


std::unordered_map<const char*, std::unique_ptr<QObject> > ApplicationServices::services_;

ApplicationServices::ApplicationServices(QObject* parent) :
    QObject(parent)
{ 
    qDebug() << "ApplicationServices";
}

ApplicationServices::~ApplicationServices()
{
    services_.clear();
    qDebug() << "~ApplicationServices";
}

void ApplicationServices::initialize()
{
    // register llm api entries
    LLMService::registerService<LlamaCppService>(LLMEnum::LLMType::LlamaCpp);    
    LLMService::registerService<OllamaService>(LLMEnum::LLMType::Ollama);

    // register model source entries
    ModelSource::registerSource<OllamaModelSource>("Ollama");
    ModelSource::registerSource<HuggingFaceModelSource>("HuggingFace");

    // add llm & RAG services
    add<LLMServices>(this);
    add<RAGService>(this);

    qDebug() << "ApplicationServices: add LLMServices:" << get<LLMServices>();
    qDebug() << "ApplicationServices: add RAGService:" << get<RAGService>();
}