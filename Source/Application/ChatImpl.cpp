#include "LLMServices.h"

#include "ChatImpl.h"

// const char* rawHumanPrompt = "human:";
// const char* rawAiPrompt = "ai:";

const char* rawHumanPrompt = "";
const char* rawAiPrompt = "";
const char* rawDefaultInitialPrompt = "";

ChatImpl::ChatImpl(LLMServices* llmservices, const QString& name, const QString& initialPrompt, bool streamed, QObject* parent) :
    Chat(llmservices, name, initialPrompt, streamed, parent),
    lastBotIndex_(-1),
    aiPrompt_("🤖 >")
{
    setObjectName(name);
    initialize();

    messagesRaw_ = !initialContext_.isEmpty() ? initialContext_ : rawDefaultInitialPrompt;
}

void ChatImpl::initialize()
{
    const auto& availableAPIs = llmservices_->getAvailableAPIs();
    if (!availableAPIs.empty())
    {
        LLMService* defaultApi = availableAPIs.front();
        if (defaultApi)
        {
            currentApi_ = defaultApi->name_;
            std::vector<LLMModel> models = defaultApi->getAvailableModels();
            if (models.size())
                currentModel_ = models.front().toString();
        }
    }

    info_["model"] = currentModel_;
    info_["stream"] = streamed_;

    history_.clear();
    emit historyChanged();
}

void ChatImpl::setApi(const QString& api)
{
    if (currentApi_ != api)
    {
        currentApi_ = api;
        emit currentApiChanged();
    }
}

void ChatImpl::setModel(const QString& model)
{
    LLMModel modelInfo = llmservices_->getModel(model);
    if (!modelInfo.name_.isEmpty())
    {
        if (modelInfo.filePath_.contains(".gguf") && currentApi_ == "Ollama")
        {
            setApi("LlamaCpp");
        }

        LLMService* api = llmservices_->get(currentApi_);
        if (api)
            api->setModel(this, model);

        if (currentModel_ != model)
        {
            currentModel_ = model;
            info_["model"] = model;
            emit currentModelChanged();
        }
    }
}

void ChatImpl::updateContent(const QString& content)
{
    // Ajoute le message utilisateur
    addContent("user", content);
    // Ajoute un bloc IA vide et mémorise son index
    addContent("assistant", "");

    emit messagesChanged();
    emit inputCleared();

    updateObject();
}

void ChatImpl::addContent(const QString& role, const QString& content)
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

        LLMService* api = llmservices_->get(currentApi_);
        messagesRaw_ += api ? api->formatMessage(this, role, content) : content + "\n";
        
        // Si c'est un message assistant, mettre à jour lastBotIndex_ et currentAIStream_
        if (!isUserContent)
        {
            lastBotIndex_ = messages_.size() - 1;
            currentAIStream_ = content;
        }
    }
    else
    {
        messages_.append("");
        lastBotIndex_ = messages_.size() - 1;
        currentAIStream_.clear();
    }

    emit messageAdded(role, content);
    emit historyChanged();
}

void ChatImpl::finalizeStream()
{
    // finalize to add the last streamed response
    if (!currentAIStream_.isEmpty())
    {
        qDebug() << "Chat::finalizeStream: ";

        LLMService* api = llmservices_->get(currentApi_);
        messagesRaw_ += api ? api->formatMessage(this, "assistant", currentAIStream_) : currentAIStream_ + "\n";

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

void ChatImpl::updateCurrentAIStream(const QString& text)
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

void ChatImpl::updateObject()
{
    info_["prompt"] = messagesRaw_;
}

void ChatImpl::addUserContent(const QString& text)
{
    addContent("user", text);
}

void ChatImpl::addAIContent(const QString& text)
{
    addContent("assistant", text);
}

QVariantList ChatImpl::historyList() const
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

QJsonObject ChatImpl::toJson() const
{
    QJsonObject json;
    json["name"] = name_;
    json["api"] = currentApi_;
    json["model"] = currentModel_;
    json["stream"] = streamed_;
    json["userPrompt"] = userPrompt_;
    json["aiPrompt"] = aiPrompt_;
    json["systemPrompt"] = initialContext_;

    QJsonArray historyArray;
    for (const auto& msg : history_)
    {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        historyArray.append(msgObj);
    }
    json["history"] = historyArray;
    return json;
}

void ChatImpl::fromJson(const QJsonObject& json)
{
    name_ = json["name"].toString();
    QString api = json["api"].toString();
    QString model = json["model"].toString();

    if (!api.isEmpty())
        currentApi_ = api;
    if (!model.isEmpty())
        currentModel_ = model;

    streamed_ = json["stream"].toBool(true);
    userPrompt_ = json["userPrompt"].toString("🧑 >");
    aiPrompt_ = json["aiPrompt"].toString("🤖 >");
    initialContext_ = json["systemPrompt"].toString();

    history_.clear();
    QJsonArray historyArray = json["history"].toArray();
    for (const auto& val : historyArray)
    {
        QJsonObject msgObj = val.toObject();
        history_.append({ msgObj["role"].toString(), msgObj["content"].toString() });
    }

    // Reconstruct messages for UI
    messages_.clear();
    if (!initialContext_.isEmpty())
        messagesRaw_ = initialContext_;
    else
        messagesRaw_ = rawDefaultInitialPrompt;

    for (const auto& msg : history_)
    {
        bool isUser = (msg.role == "user");
        messages_.append(QString("%1 %2\n").arg(isUser ? userPrompt_ : aiPrompt_, msg.content));

        if (llmservices_)
        {
            LLMService* apiEntry = llmservices_->get(currentApi_);
            messagesRaw_ += apiEntry ? apiEntry->formatMessage(this, msg.role, msg.content) : msg.content + "\n";
        }
    }

    emit historyChanged();
    emit messagesChanged();
    emit currentApiChanged();
    emit currentModelChanged();
}

QString ChatImpl::getFullConversation() const
{
    QString result;
    for (const auto& msg : history_)
    {
        QString roleName = (msg.role == "user") ? "USER" : "AI";
        result += QString("[%1]:\n%2\n\n").arg(roleName, msg.content);
    }
    return result.trimmed();
}

QString ChatImpl::getUserPrompts() const
{
    QString result;
    for (const auto& msg : history_)
    {
        if (msg.role == "user")
            result += msg.content + "\n\n";
    }
    return result.trimmed();
}

QString ChatImpl::getBotResponses() const
{
    QString result;
    for (const auto& msg : history_)
    {
        if (msg.role == "assistant")
            result += msg.content + "\n\n";
    }
    return result.trimmed();
}