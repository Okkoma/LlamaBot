#pragma once

#include "../../Source/Application/LLMService.h"

// Mock amélioré pour tester les interactions avec les services LLM
class MockLLMService : public LLMService
{
public:
    MockLLMService(LLMEnum::LLMType type, LLMServices* s, const QString& name) :
        LLMService(static_cast<int>(type), s, name) { }

    MockLLMService(LLMServices* llmservices, const QVariantMap& params) :
        LLMService(llmservices, params) { }

    std::vector<LLMModel> getAvailableModels() const override 
    {
        return models_;
    }

    void addModel(const QString& name, const QString& num_params="7B", const QString& filePath="") 
    {
        LLMModel model;
        model.name_ = name;
        model.num_params_ = num_params;
        model.filePath_ = filePath;
        models_.push_back(model);
    }

    bool isReady() const override { return ready_; }
    void setReady(bool ready) { ready_ = ready; }

    std::vector<LLMModel> models_;
    bool ready_{false};
};
