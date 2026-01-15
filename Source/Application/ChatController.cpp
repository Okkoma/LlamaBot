#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QBuffer>

#include "LLMService.h"
#include "ChatImpl.h"

#include "ChatController.h"

ChatController::ChatController(LLMServices* llmservices, QObject* parent) :
    QObject(parent),
    llmServices_(llmservices),
    currentChat_(nullptr),
    chatCounter_(0),
    ragService_(new RAGService(llmservices, this))
{
    // Fix: Connect LLMServices signals to ChatController signals to notify QML
    connect(llmServices_, &LLMServices::defaultContextSizeChanged, this, &ChatController::defaultContextSizeChanged);
    connect(llmServices_, &LLMServices::autoExpandContextChanged, this, &ChatController::autoExpandContextChanged);

    // Try to load existing chats
    loadChats();

    // specific case for first run
    if (chats_.isEmpty())
        createChat();

    const std::vector<LLMService*>& apiList = llmServices_->getAPIs();
    if (apiList.size() && apiList.front())
    {
        // If loaded chats have no API set (or were created from scratch), set default
        if (currentChat_ && currentChat_->getCurrentApi() == "none")
            setAPI(apiList.front()->name_);
    }
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
        Chat* chat = chats_[i];
        QVariantMap chatInfo;
        chatInfo["index"] = i;
        chatInfo["name"] = chat->getName();
        chatInfo["model"] = chat->getCurrentModel();
        // Add a reference to the Chat object itself
        chatInfo["chatObject"] = QVariant::fromValue(chat);
        list.append(chatInfo);
    }
    return list;
}

int ChatController::currentChatIndex() const
{
    return chats_.indexOf(currentChat_);
}

void ChatController::notifyUpdatedChat(Chat* chat)
{
    qDebug() << "ChatController::notifyUpdatedChat";
    emit chatContentUpdated(chat);
    checkChatsProcessingFinished();
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
        saveChats();
    }
}

void ChatController::connectAPIsSignals()
{
    const std::vector<LLMService*>& apiList = llmServices_->getAPIs();
    for (LLMService* api : apiList)
    {
        QObject::connect(api, SIGNAL(modelLoadingStarted(const QString&)), this, SIGNAL(loadingStarted()));
        QObject::connect(api, SIGNAL(modelLoadingFinished(const QString&, bool)), this, SIGNAL(loadingFinished()));
    }
}

void ChatController::createChat()
{
    chatCounter_++;
    QString chatName = QString("Chat %1").arg(chatCounter_);
    Chat* chat = new ChatImpl(llmServices_, chatName, "", true, this);
    chats_.append(chat);
    currentChat_ = chat;

    QObject::connect(chat, &Chat::processingFinished, this, &ChatController::notifyUpdatedChat);

    // Save new chat creation
    saveChats();

    emit chatListChanged();
    emit currentChatChanged();

    chat->setContextSize(llmServices_->getDefaultContextSize());
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

        QObject::disconnect(
            chatToRemove, &Chat::processingFinished, this, &ChatController::notifyUpdatedChat);

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
        saveChats();
        emit chatListChanged();
    }
}

void ChatController::renameChat(int index, const QString& name)
{
    if (index >= 0 && index < chats_.size())
    {
        chats_[index]->setName(name);
        saveChats();
        emit chatListChanged();
    }
}

void ChatController::sendMessage(const QString& text)
{
    if (!currentChat_)
        return;

    LLMService* api = llmServices_->get(currentChat_->getCurrentApi());
    if (api)
    {
        qDebug() << "ChatController::sendMessage ... start loading spinner";
        emit loadingStarted();

        QString prompt = text;

        if (ragEnabled_ && ragService_)
        {
            QString context = ragService_->retrieveContext(text);
            if (!context.isEmpty())
            {
                // Augment prompt
                prompt = QString("Uses the following context to answer the user question:\n%1\n\nUser Question: %2")
                             .arg(context).arg(text);
            }
        }

        // Add assets (images, audio)
        currentChat_->setAssets(pendingAssets_);

        llmServices_->post(api, currentChat_, prompt, true);

        clearAssets();
    }
}

void ChatController::stopGeneration()
{
    if (currentChat_)
        llmServices_->stop(currentChat_);
}

QVariantList ChatController::getAvailableModels()
{
    QVariantList models;

    // Get models from current API or all APIs
    LLMService* currentApi = currentChat_ ? llmServices_->get(currentChat_->getCurrentApi()) : nullptr;
    std::vector<LLMModel> modelList = llmServices_->getAvailableModels(currentApi);

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

    const std::vector<LLMService*>& apiList = llmServices_->getAPIs();
    for (LLMService* api : apiList)
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

QString ChatController::getChatsFilePath() const
{
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataLocation);
    if (!dir.exists())
        dir.mkpath(".");

    return dir.filePath("chats.json");
}

void ChatController::saveChats()
{
    QFile file(getChatsFilePath());
    if (!file.open(QIODevice::WriteOnly))
    {
        qWarning() << "Could not open chats file for writing:" << file.fileName();
        return;
    }

    QJsonArray array;
    for (Chat* chat : chats_)
    {
        array.append(chat->toJson());
    }

    QJsonDocument doc(array);
    file.write(doc.toJson());
    file.close();
    qDebug() << "Chats saved to" << file.fileName();
}

void ChatController::loadChats()
{
    qDebug() << "ChatController::loadChats()";
    
    chats_.clear();

    QFile file(getChatsFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        return; // existing file not found or couldn't be read
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
    {
        qWarning() << "Invalid chats file format";
        return;
    }

    QJsonArray array = doc.array();
    for (const auto& val : array)
    {
        if (val.isObject())
        {
            chatCounter_++;
            // Create chat but don't append to list immediately to avoid signals issues or just do standard creation
            // then overwrite Actually standard creation connects signals which is good. But standard creation appends
            // to list. Let's create chat manually here to control initialization from JSON.

            Chat* chat = new ChatImpl(llmServices_, "", "", true, this);
            chat->fromJson(val.toObject());

            // Ensure unique name handling if needed, but for now trust JSON or just let it be.
            // If chat name is empty (legacy?), give it a name.
            if (chat->getName().isEmpty())
                chat->setName(QString("Chat %1").arg(chatCounter_));

            chats_.append(chat);

            // Connect signals
            QObject::connect(chat, &Chat::processingFinished, this, &ChatController::notifyUpdatedChat);
        }
    }

    if (!chats_.isEmpty())
    {
        currentChat_ = chats_.last();
        emit currentChatChanged();
        emit chatListChanged();
    }
}

void ChatController::setDefaultContextSize(int size)
{
    if (llmServices_->getDefaultContextSize() != size)
        llmServices_->setDefaultContextSize(size);        
}

void ChatController::setAutoExpandContext(bool enabled)
{
    if (llmServices_->getAutoExpandContext() != enabled)
        llmServices_->setAutoExpandContext(enabled);
}

QString ChatController::imageToBase64(const QString& imagePath) const
{
    QFile file(imagePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return QString();
    
    QByteArray fileData = file.readAll();
    file.close();
    
    // Détecter le type MIME
    QString mimeType = "image/png";
    QString extension = QFileInfo(imagePath).suffix().toLower();
    if (extension == "jpg" || extension == "jpeg")
        mimeType = "image/jpeg";
    else if (extension == "gif")
        mimeType = "image/gif";
    else if (extension == "webp")
        mimeType = "image/webp";
    
    return QString("data:%1;base64,%2")
        .arg(mimeType)
        .arg(QString::fromLatin1(fileData.toBase64()));
}

void ChatController::addAsset(const QString& assetPath)
{
    if (assetPath.isEmpty())
        return;
    
    qDebug() << "ChatController::addAsset:" << assetPath;

    // Convertir l'image en base64
    QString base64Image = imageToBase64(assetPath);
    if (base64Image.isEmpty())
    {
        qWarning() << "Impossible de convertir l'image en base64:" << assetPath;
        return;
    }
    
    // Ajouter à la liste temporaire
    QVariantMap asset;
    asset["type"] = "image";
    asset["base64"] = base64Image;
    asset["path"] = assetPath;
    asset["name"] = QFileInfo(assetPath).fileName();
    pendingAssets_.append(asset);

    emit pendingAssetsChanged();
}

void ChatController::addAssetBase64(const QString& assetContent)
{
    if (assetContent.isEmpty())
        return;

    qDebug() << "ChatController::assetContent:" << assetContent;
    
    QVariantMap asset;
    asset["type"] = "image";
    asset["base64"] = assetContent;
    asset["path"] = "";
    asset["name"] = "Image collée";
    pendingAssets_.append(asset);

    emit pendingAssetsChanged();
}

void ChatController::removeAsset(int index)
{
    if (index >= 0 && index < pendingAssets_.size())
    {
        pendingAssets_.removeAt(index);
        emit pendingAssetsChanged();
    }
}

void ChatController::clearAssets()
{
    if (!pendingAssets_.isEmpty())
    {
        pendingAssets_.clear();
        emit pendingAssetsChanged();
    }
}
