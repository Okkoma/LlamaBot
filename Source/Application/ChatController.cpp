#include "ChatController.h"

#include <QDebug>

ChatController::ChatController(LLMService* service, QObject* parent) :
    QObject(parent), service_(service), currentChat_(nullptr), chatCounter_(0)
{
    // Create the first chat
    createChat();
}

ChatController::~ChatController()
{
    // Chats will be deleted automatically as they are parented to this
}

QVariantList ChatController::chatList() const
{
    QVariantList list;
    for (int i = 0; i < chats_.size(); ++i)
    {
        QVariantMap chatInfo;
        chatInfo["index"] = i;
        chatInfo["name"] = chats_[i]->name_;
        chatInfo["model"] = chats_[i]->currentModel();
        chatInfo["messageCount"] = chats_[i]->history_.size();
        list.append(chatInfo);
    }
    return list;
}

int ChatController::currentChatIndex() const
{
    return chats_.indexOf(currentChat_);
}

void ChatController::checkChatsProcessingFinished() 
{
    int count = 0;
    for (Chat* chat : chats_)
    {
        if (!chat->isProcessing())
            count++;
    }

    if (count == chats_.size())
    {
        qDebug() << "ChatController::checkChatsProcessingFinished ... end loading spinner";
        emit loadingFinished();
    }
}

void ChatController::connectAPIsSignals()
{
    const std::vector<LLMAPIEntry*>& apiList = service_->getAPIs();
    for (LLMAPIEntry* api : apiList)
    {
        QObject::connect(api, SIGNAL(modelLoadingStarted(const QString&)), this, SIGNAL(loadingStarted()));
        QObject::connect(api, SIGNAL(modelLoadingFinished(const QString&, bool)), this, SIGNAL(loadingFinished()));
    }
}

void ChatController::createChat()
{
    chatCounter_++;
    QString chatName = QString("Chat %1").arg(chatCounter_);
    Chat* chat = new Chat(service_, chatName, "", true, this);
    chats_.append(chat);

    currentChat_ = chat;

    QObject::connect(chat, &Chat::processingFinished, this, &ChatController::checkChatsProcessingFinished);
    
    emit chatListChanged();
    emit currentChatChanged();
}

void ChatController::switchToChat(int index)
{
    if (index >= 0 && index < chats_.size())
    {
        if (currentChat_ != chats_[index])
        {           
            currentChat_ = chats_[index];

            emit currentChatChanged();
        }
    }
}

void ChatController::deleteChat(int index)
{
    if (index >= 0 && index < chats_.size() && chats_.size() > 1)
    {
        Chat* chatToRemove = chats_[index];

        QObject::disconnect(chatToRemove, &Chat::processingFinished, this, &ChatController::checkChatsProcessingFinished);

        chats_.removeAt(index);

        // Update current chat if needed
        if (currentChat_ == chatToRemove)
        {
            if (index < chats_.size())
                currentChat_ = chats_[index];
            else if (!chats_.isEmpty())
                currentChat_ = chats_.last();
            else
                currentChat_ = nullptr;

            emit currentChatChanged();
        }

        chatToRemove->deleteLater();
        emit chatListChanged();
    }
}

void ChatController::renameChat(int index, const QString& name)
{
    if (index >= 0 && index < chats_.size())
    {
        chats_[index]->name_ = name;
        emit chatListChanged();
    }
}

void ChatController::sendMessage(const QString& text)
{
    if (!currentChat_)
        return;

    LLMAPIEntry* api = service_->get(currentChat_->currentApi());
    if (api)
    {
        qDebug() << "ChatController::sendMessage ... start loading spinner";
        emit loadingStarted();
        service_->post(api, currentChat_, text, true);
    }
}

void ChatController::stopGeneration()
{
    if (currentChat_)
    {
        service_->stopStream(currentChat_);        
    }
}

QVariantList ChatController::getAvailableModels()
{
    QVariantList models;

    // Get models from current API or all APIs
    LLMAPIEntry* currentApi = currentChat_ ? service_->get(currentChat_->currentApi()) : nullptr;
    std::vector<LLMModel> modelList = service_->getAvailableModels(currentApi);

    for (const LLMModel& model : modelList)
    {
        QVariantMap modelInfo;
        modelInfo["name"] = model.toString();
        modelInfo["filePath"] = model.filePath_;
        modelInfo["params"] = model.num_params_;
        models.append(modelInfo);
    }

    return models;
}

QVariantList ChatController::getAvailableAPIs()
{
    QVariantList apis;

    const std::vector<LLMAPIEntry*>& apiList = service_->getAPIs();
    for (LLMAPIEntry* api : apiList)
    {
        QVariantMap apiInfo;
        apiInfo["name"] = api->name_;
        apiInfo["ready"] = api->isReady();
        apis.append(apiInfo);
    }

    return apis;
}

void ChatController::setModel(const QString& modelName)
{
    if (currentChat_)
    {
        qDebug() << "ChatController::setModel" << modelName;

        currentChat_->setModel(modelName);
        emit currentChatChanged(); // Notify to update UI
    }
}

void ChatController::setAPI(const QString& apiName)
{
    if (currentChat_)
    {
        qDebug() << "ChatController::setAPI" << apiName;

        currentChat_->setApi(apiName);
        emit currentChatChanged();     // Notify to update UI
        emit availableModelsChanged(); // Notify that available models list has changed

        connectAPIsSignals();
    }
}
