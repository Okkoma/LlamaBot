#pragma once

#include "define.h"

class LLMEnum : public QObject
{
    Q_OBJECT

public:
    enum LLMType
    {
        LlamaCpp,
        Ollama,
        OpenAI,
    };
    Q_ENUM(LLMType)
};

class LLMModel
{
public:
    LLMModel() = default;

    QString toString() const { return name_ + ":" + num_params_; }

    QString name_;
    QString format_;
    QString num_params_;
    QString vendor_;
    QString filePath_;
};
