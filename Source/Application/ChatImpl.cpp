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
    if (llmservices)
        getData()->n_ctx_ = llmservices->getDefaultContextSize();

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
    if (currentApi_ != api && llmservices_->get(api))
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
        history_.append({ role, content, getAssets() });

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
            currentAIRole_ = role;
        }
    }
    else
    {
        messages_.append("");
        lastBotIndex_ = messages_.size() - 1;
        currentAIStream_.clear();
        currentAIRole_ = role;
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
        // Map "thought" back to "assistant" for raw storage if needed, but wait.
        // If we want the raw prompt to be correct, we should probably have kept the tags.
        // Actually, LLM handles its own history in LlamaCppService. 
        // This messagesRaw_ is for OTHER services or reloading.
        
        QString roleForRaw = (currentAIRole_ == "thought") ? "assistant" : currentAIRole_;
        QString contentForRaw = currentAIStream_;
        if (currentAIRole_ == "thought") contentForRaw = "<think>" + contentForRaw + "</think>";

        messagesRaw_ += api ? api->formatMessage(this, roleForRaw, contentForRaw) : contentForRaw + "\n";

        // History is already updated during streaming in updateCurrentAIStream()
        if (history_.isEmpty() || history_.last().role != currentAIRole_ || history_.last().content != currentAIStream_)
        {
            qWarning() << "Chat::finalizeStream: history was not updated during streaming, adding now";
            history_.append({ currentAIRole_, currentAIStream_ });
            emit historyChanged();
        }

        currentAIStream_.clear();
        currentAIRole_ = "assistant";
        emit streamFinishedSignal();
    }
}

void ChatImpl::updateCurrentAIStream(const QString& text)
{
    if (text.isEmpty())
        return;

    currentAIStream_ += text;

    // Detection of <think>
    if (currentAIStream_.contains("<think>"))
    {
        currentAIStream_.remove("<think>");
        currentAIRole_ = "thought";
        if (!history_.isEmpty() && history_.last().role == "assistant")
        {
            history_.last().role = "thought";
        }
    }

    // Detection of </think>
    if (currentAIStream_.contains("</think>"))
    {
        int tagPos = currentAIStream_.indexOf("</think>");
        QString before = currentAIStream_.left(tagPos);
        QString after = currentAIStream_.mid(tagPos + 8); // 8 = length of </think>

        // Finalize "thought" part
        if (!history_.isEmpty() && history_.last().role == "thought")
        {
            history_.last().content = before;
            messages_[lastBotIndex_] = QString("%1 %2\n").arg(aiPrompt_, before);
        }

        // Start "assistant" part
        currentAIStream_ = after;
        currentAIRole_ = "assistant";

        // Add new assistant entry in history
        history_.append({ "assistant", currentAIStream_ });
        
        // Add new entry in messages_
        messages_.append(QString("%1 %2\n").arg(aiPrompt_, currentAIStream_));
        lastBotIndex_ = messages_.size() - 1;

        emit messagesChanged();
        emit historyChanged();
        emit streamUpdated(text);
        emit contextSizeUsedChanged();
        return;
    }

    // some sanitization (only if at start)
    if (currentAIStream_.length() < 5)
    {
        if (currentAIStream_.startsWith("|"))
            currentAIStream_.removeFirst();

        if (currentAIStream_.startsWith(">"))
        {
            currentAIStream_.removeFirst();
            if (currentAIStream_.startsWith("\n"))
                currentAIStream_.removeFirst();
        }
    }

    messages_[lastBotIndex_] = QString("%1 %2\n").arg(aiPrompt_, currentAIStream_);

    // Update history in real-time for QML display
    if (!history_.isEmpty() && history_.last().role == currentAIRole_)
    {
        history_.last().content = currentAIStream_;
    }
    else
    {
        history_.append({ currentAIRole_, currentAIStream_ });
    }

    emit streamUpdated(text);
    emit messagesChanged();
    emit historyChanged();
    emit contextSizeUsedChanged();
}

void ChatImpl::updateObject()
{
    info_["prompt"] = messagesRaw_;
}

QVariantList ChatImpl::historyList() const
{
    QVariantList list;
    for (const auto& msg : history_)
    {
        QVariantMap map;
        map["role"] = msg.role;
        map["content"] = msg.content;
        map["assets"] = msg.assets;
        list.append(map);
    }
    return list;
}

QJsonObject ChatImpl::toJson() const
{
    QJsonObject json;
    json["n_ctx"] = getData()->n_ctx_;
    json["n_ctx_used"] = getData()->n_ctx_used_;
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
        if (msg.assets.size())
        {
            QJsonArray assetsArray;
            for (const auto& asset : msg.assets)
                assetsArray.append(asset.toJsonObject());
            msgObj["assets"] = assetsArray;
        }
        historyArray.append(msgObj);
    }
    json["history"] = historyArray;
    return json;
}

void ChatImpl::fromJson(const QJsonObject& json)
{
    getData()->n_ctx_ = json["n_ctx"].toInt();
    getData()->n_ctx_used_ = json["n_ctx_used"].toInt();

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
        history_.append({ msgObj["role"].toString(), msgObj["content"].toString(), 
                    msgObj["assets"].toVariant().toList() });
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

    qDebug() << "ChatImpl::fromJson" << this;

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
