#include <QDebug>

#include "Chat.h"
#include "LLMService.h"

// const char* rawHumanPrompt = "human:";
// const char* rawAiPrompt = "ai:";

const char* rawHumanPrompt = "";
const char* rawAiPrompt = "";
const char* rawDefaultInitialPrompt = "";

Chat::Chat(LLMService* service, const QString& name, const QString& initialPrompt, bool streamed, QObject* parent) :
    QObject(parent),
    service_(service),
    streamed_(streamed),
    lastBotIndex_(-1),
    name_(name),
    currentApi_("none"),
    currentModel_("none"),
    userPrompt_("🧑 >"),
    aiPrompt_("🤖 >")
{
    initialize();

    rawMessages_ = !initialPrompt.isEmpty() ? initialPrompt : rawDefaultInitialPrompt;
}

void Chat::initialize()
{
    LLMAPIEntry* defaultApi = service_->getAvailableAPIs().front();
    if (defaultApi)
    {
        currentApi_ = defaultApi->name_;
        std::vector<LLMModel> models = defaultApi->getAvailableModels();
        if (models.size())
            currentModel_ = models.front().toString();
    }

    jsonObject_["model"] = currentModel_;
    jsonObject_["stream"] = streamed_;

    history_.clear();
    emit historyChanged();
}

void Chat::setApi(const QString& api)
{
    if (currentApi_ != api)
    {
        currentApi_ = api;
        emit currentApiChanged();
    }
}

void Chat::setModel(const QString& model)
{
    LLMModel* modelptr = service_->getModel(model);
    qDebug() << "Chat::setModel: modelptr:" << modelptr << "model:" << model;
    if (modelptr)
    {
        if (modelptr->filePath_.contains(".gguf") && currentApi_ == "Ollama")
        {
            qDebug() << "Chat::setModel: Detected local model file, switching API to LlamaCpp";
            setApi("LlamaCpp");
        }

        LLMAPIEntry* api = service_->get(currentApi_);
        if (api)
            api->setModel(this, model);

        if (currentModel_ != model)
        {
            currentModel_ = model;
            jsonObject_["model"] = model;
            emit currentModelChanged();
        }
    }
}

void Chat::updateContent(const QString& content)
{
    // Ajoute le message utilisateur
    addContent("user", content);
    // Ajoute un bloc IA vide et mémorise son index
    addContent("assistant", "");

    emit messagesChanged();
    emit inputCleared();

    updateObject();
}

void Chat::addContent(const QString& role, const QString& content)
{
    qDebug() << "Chat::addContent: role:" << role << "content:" << content;

    bool isUserContent = (role == "user");
    if (isUserContent)
        finalizeStream();

    // Add to history
    if (!content.isEmpty())
        history_.append({ role, content });

    if (!content.isEmpty())
    {
        messages_.append(QString("%1 %2\n").arg(isUserContent ? userPrompt_ : aiPrompt_, content));

        LLMAPIEntry* api = service_->get(currentApi_);
        rawMessages_ += api ? api->formatMessage(this, role, content) : content + "\n";
    }
    else
    {
        messages_.append("");
        lastBotIndex_ = messages_.size() - 1;
    }

    emit messageAdded(role, content);
    emit historyChanged();
}

void Chat::finalizeStream()
{
    // finalize to add the last streamed response
    if (!currentAIStream_.isEmpty())
    {
        qDebug() << "Chat::finalizeStream: ";

        LLMAPIEntry* api = service_->get(currentApi_);
        rawMessages_ += api ? api->formatMessage(this, "assistant", currentAIStream_) : currentAIStream_ + "\n";

        // History is already updated during streaming in updateCurrentAIStream()
        // No need to append again, just verify it's there
        if (history_.isEmpty() || history_.last().role != "assistant" || history_.last().content != currentAIStream_)
        {
            // Fallback: if for some reason history wasn't updated during streaming
            qWarning() << "Chat::finalizeStream: history was not updated during streaming, adding now";
            history_.append({ "assistant", currentAIStream_ });
            emit historyChanged();
        }

        currentAIStream_.clear();
        emit streamFinishedSignal();
    }
}

void Chat::updateCurrentAIStream(const QString& text)
{
    if (!text.isEmpty())
    {
        currentAIStream_ += text;

        // some sanitization
        if (currentAIStream_.startsWith("|"))
            currentAIStream_.removeFirst();

        if (currentAIStream_.startsWith(">"))
        {
            currentAIStream_.removeFirst();
            if (currentAIStream_.startsWith("\n"))
                currentAIStream_.removeFirst();
        }

        messages_[lastBotIndex_] = QString("%1 %2\n").arg(aiPrompt_, currentAIStream_);

        // Update history in real-time for QML display
        // Check if the last entry is an assistant message (streaming placeholder)
        if (!history_.isEmpty() && history_.last().role == "assistant" && history_.last().content.isEmpty())
        {
            // Update the existing empty assistant entry
            history_.last().content = currentAIStream_;
        }
        else if (!history_.isEmpty() && history_.last().role == "assistant")
        {
            // Update existing assistant message (already streaming)
            history_.last().content = currentAIStream_;
        }
        else
        {
            // No assistant entry yet, create one
            history_.append({ "assistant", currentAIStream_ });
        }

        emit streamUpdated(text); // Signal for partial updates
        emit messagesChanged();   // Signal for full text updates
        emit historyChanged();    // Signal that history has been updated
    }
}

void Chat::setProcessing(bool processing)
{
    processing_ = processing;
    if (processing) 
        emit processingStarted();
    else
        emit processingFinished();
}

void Chat::updateObject()
{
    jsonObject_["prompt"] = rawMessages_;
}

void Chat::addUserContent(const QString& text)
{
    addContent("user", text);
}

void Chat::addAIContent(const QString& text)
{
    addContent("assistant", text);
}

QVariantList Chat::historyList() const
{
    QVariantList list;
    for (const auto& msg : history_)
    {
        QVariantMap map;
        map["role"] = msg.role;
        map["content"] = msg.content;
        list.append(map);
    }
    return list;
}