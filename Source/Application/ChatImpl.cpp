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
    messages_.clear();
    emit messagesChanged();
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
}

void ChatImpl::addContent(const QString& role, const QString& content)
{
    qDebug() << "Chat::addContent: role:" << role << "content:" << content;

    bool isUserContent = (role == "user");
    if (isUserContent)
        finalizeStream();

    addMessage(role, content, isUserContent ? getAssets() : QVariantList());
    messages_.append(QString("%1 %2\n").arg(isUserContent ? userPrompt_ : aiPrompt_).arg(content));

    // Si c'est un message assistant, mettre à jour lastBotIndex_ et currentAIStream_
    if (!isUserContent)
    {
        lastBotIndex_ = messages_.size() - 1;
        currentAIStream_ = content;
        currentAIRole_ = role;
    }

    emit messageAdded(role, content);
    emit messagesChanged();
}

void ChatImpl::finalizeStream()
{
    // finalize to add the last streamed response
    if (!currentAIStream_.isEmpty())
    {
        qDebug() << "Chat::finalizeStream: ";

        // History is already updated during streaming in updateCurrentAIStream()
        if (history_.isEmpty() || history_.last().role_ != currentAIRole_ || history_.last().content_ != currentAIStream_)
        {
            qWarning() << "Chat::finalizeStream: history was not updated during streaming, adding now";
            addMessage(currentAIRole_, currentAIStream_);
            emit messagesChanged();
        }

        currentAIStream_.clear();
        currentAIRole_ = "assistant";
        emit streamFinishedSignal();
    }
}

void ChatImpl::sanitizeStream(QString& text)
{
    if (text.startsWith("|"))
        text.removeFirst();

    if (text.startsWith(">"))
    {
        text.removeFirst();
        if (text.startsWith("\n"))
            text.removeFirst();
    }
}

void ChatImpl::updateCurrentAIStream(const QString& text)
{
    if (text.isEmpty())
        return;

    bool finalized = text.endsWith("<end>");
    if (finalized)
        currentAIStream_ = text.chopped(5);
    else 
        currentAIStream_ += text;

    //qDebug() << "Chat::updateCurrentAIStream:" << text;
    
    sanitizeStream(currentAIStream_);

    //qDebug() << "Chat::updateCurrentAIStream:" << currentAIStream_;

    // find all "thought" or default assistant messages in the stream
    struct MessageStringView
    {
        MessageStringView(const QString& role, const QStringView& content) : role_(role), content_(content) { }
        QString role_;
        QStringView content_;
    };
    QVector<MessageStringView> messages;
    long long currentIndex = 0L;
    QStringView currentContent = currentAIStream_;
    while (currentIndex < currentContent.length())
    {
        currentContent.slice(currentIndex);
        if (currentContent.isEmpty())
            break;

        qsizetype iThinkBegin = currentContent.indexOf(QLatin1StringView("<think>"));
        qsizetype iThinkEnd = currentContent.indexOf(QLatin1StringView("</think>"), iThinkBegin == -1 ? 0 : iThinkBegin + 7);
        currentIndex = iThinkEnd != -1 ? iThinkEnd + 8 : currentContent.length();

        // add default message before this thought
        if (iThinkBegin > 0)
            messages.emplace_back("assistant", currentContent.first(iThinkBegin));

        // "think" END tag found !
        if (iThinkEnd != -1)
        {
            // "think" BEGIN tag found !
            if (iThinkBegin != -1)            
                messages.emplace_back("thought", currentContent.mid(iThinkBegin + 7, iThinkEnd - iThinkBegin - 7));
            // here, no "think" BEGIN tag so the thought message should begin at index 0
            else
                messages.emplace_back("thought", currentContent.left(iThinkEnd));

            // switch to default assistant role
            currentIndex = iThinkEnd + 8;
            currentAIRole_ = "assistant";
        }
        // NO "think" END tag and "think" BEGIN tag found !
        else if (iThinkBegin != -1)
        {
            // switch to "thought" role
            currentAIRole_ = "thought";
            messages.emplace_back(currentAIRole_, currentContent.sliced(iThinkBegin + 7));
        }
        // NO 'think" tags
        else
        {
            // no think tags, keep on the current role (should be "thought" or default)
            messages.emplace_back(currentAIRole_, currentContent);
            // end of the loop
            currentIndex = currentContent.length();
        }
    }

    // where to write the messages found ?
    // at the begin of the ai response, a new message is appended in messages_
    int msgindex = lastBotIndex_;
    for (MessageStringView& msg : messages)
    {
        if (msgindex < messages_.size())
        {
            modifyMessage(msgindex, msg.role_, msg.content_.toString());
            messages_[msgindex] = QString("%1 %2\n").arg(aiPrompt_).arg(msg.content_.toString());
        }
        else
        {
            addMessage(msg.role_, msg.content_.toString());
            messages_.emplace_back(QString("%1 %2\n").arg(aiPrompt_).arg(msg.content_.toString()));
        }
        msgindex++;
    }

    if (finalized)
        currentAIStream_.clear();

    emit messagesChanged();
    emit streamUpdated(text);
    emit contextSizeUsedChanged();
}

QString ChatImpl::getFormattedHistory() const
{
    LLMService* api = llmservices_->get(currentApi_);
    return api ? api->formatMessages(this) : QString();
}

QString ChatImpl::getFormattedMessage(const QString& role, qsizetype position) const
{
    LLMService* api = llmservices_->get(currentApi_);
    if (api)
    {
        qsizetype index, limit;
        int direction;
        if (position < 0)
        {
            direction = -1; 
            limit = -1;
            index = std::clamp(history_.size() + position, 0LL, history_.size()-1);
        }
        else
        {
            direction = 1; 
            limit = history_.size(); 
            index = std::clamp(position, 0LL, history_.size()-1);
        }
      
        for (; index != limit; index += direction)
        {
            const ChatMessage& message = history_.at(index);
            if (message.role_ == role)
                return api->formatMessage(this, index);
        }
    }
    return {};
}

QJsonObject ChatImpl::toJson() const
{
    const ChatData* data = getData();
    QJsonObject json;
    json["id"] = getId();
    json["n_ctx"] = data->n_ctx_;
    json["n_ctx_used"] = data->n_ctx_used_;
    json["name"] = name_;
    // TODO : fix api and model with the last api/model used by this chat
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
        msgObj["role"] = msg.role_;
        msgObj["content"] = msg.content_;
        if (msg.assets_.size())
        {
            QJsonArray assetsArray;
            for (const auto& asset : msg.assets_)
                assetsArray.append(asset.toJsonObject());
            msgObj["assets"] = assetsArray;
        }
        historyArray.append(msgObj);
    }
    json["history"] = historyArray;

    if (data->context_tokens_.size())
    {
        QJsonArray jsonArray;
        for (int token : data->context_tokens_) 
            jsonArray.append(token);            
        json["tokenized_content"] = jsonArray;
        qDebug() << "ChatImpl::toJson : set tokenized_content: size:" << data->context_tokens_.size();
    }

    return json;
}

void ChatImpl::fromJson(const QJsonObject& json)
{
    ChatData* data = getData();
    setId(json["id"].toString());
    data->n_ctx_ = json["n_ctx"].toInt();
    data->n_ctx_used_ = json["n_ctx_used"].toInt();

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

    if (json.contains("tokenized_content") && json["tokenized_content"].isArray())
    {
        QJsonArray jsonArray = json["tokenized_content"].toArray();
        for (const QJsonValue& token : jsonArray)
        {
            if (token.isDouble())
                data->context_tokens_.push_back(token.toInt());
        }
        qDebug() << "ChatImpl::fromJson : get tokenized_content size:" << data->context_tokens_.size();
    }

    // Reconstruct messages for UI
    messages_.clear();
    for (const auto& msg : history_)
        messages_.append(QString("%1 %2\n").arg(msg.role_ == "user" ? userPrompt_ : aiPrompt_).arg(msg.content_));

    qDebug() << "ChatImpl::fromJson" << this;

    emit messagesChanged();
    emit currentApiChanged();
    emit currentModelChanged();
}

QString ChatImpl::getFullConversation() const
{
    QString result;
    for (const auto& msg : history_)
    {
        QString roleName = (msg.role_ == "user") ? "USER" : "AI";
        result += QString("[%1]:\n%2\n\n").arg(roleName).arg(msg.content_);
    }
    return result.trimmed();
}

QString ChatImpl::getUserPrompts() const
{
    QString result;
    for (const auto& msg : history_)
    {
        if (msg.role_ == "user")
            result += msg.content_ + "\n\n";
    }
    return result.trimmed();
}

QString ChatImpl::getBotResponses() const
{
    QString result;
    for (const auto& msg : history_)
    {
        if (msg.role_ == "assistant")
            result += msg.content_ + "\n\n";
    }
    return result.trimmed();
}
