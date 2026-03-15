#include <QFile>
#include <QImage>
#include <QBuffer>
#include <QtConcurrent/QtConcurrent>

#include "ApplicationServices.h"
#include "LLMService.h"
#include "ChatImpl.h"
#include "ChatStorageLocal.h"
#include "ChatStorageDistant.h"
#include "RAGService.h"

#include "ChatController.h"


ChatController::ChatController(QObject* parent) :
    QObject(parent),
    currentChat_(nullptr),
    chatCounter_(0)
{
    initialize();
}

void ChatController::initialize()
{
    llmServices_ = ApplicationServices::get<LLMServices>();

    // Try to load existing chats
    localStore_ = new ChatStorageLocal(llmServices_);
    cloudStore_ = new ChatStorageDistant(llmServices_);

    loadChats();

    // specific case for first run
    if (chats_.isEmpty())
        createChat("none", "none");

    const std::vector<LLMService*>& apiList = llmServices_->getAPIs();
    if (apiList.size() && apiList.front())
    {
        // If loaded chats have no API set (or were created from scratch), set default
        if (currentChat_ && currentChat_->getCurrentApi() == "none")
            setAPI(apiList.front()->name_);
    }    

    if (currentAPI_.isEmpty())
        currentAPI_ = llmServices_->getAPIs().front()->name_;

    setCurrentApi();
    setCurrentModel();

    // Fix: Connect LLMServices signals to ChatController signals to notify QML
    connect(llmServices_, &LLMServices::defaultContextSizeChanged, this, &ChatController::defaultContextSizeChanged);
    connect(llmServices_, &LLMServices::autoExpandContextChanged, this, &ChatController::autoExpandContextChanged);    
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

void ChatController::createChat(const QString& api, const QString& model)
{
    chatCounter_++;
    QString chatName = QString("Chat %1").arg(chatCounter_);
    Chat* chat = new ChatImpl(llmServices_, chatName, "", true, api, model, this);
    chats_.append(chat);

    currentChat_ = chat;

    QObject::connect(chat, &Chat::processingFinished, this, &ChatController::notifyUpdatedChat);

    // Save new chat creation
    saveChats();

    setCurrentApi(currentChat_->getCurrentApi());
    setCurrentModel(currentChat_->getCurrentModel());

    emit chatListChanged();
    emit currentChatChanged();

    chat->setContextSize(llmServices_->getDefaultContextSize());
}

void ChatController::switchToChat(int index)
{
    Chat* chat = currentChat_;

    if (index >= 0 && index < chats_.size())
        chat = chats_[index];
    else if (!chats_.isEmpty())
        chat = chats_.last();
    else 
        chat = nullptr;

    if (chat != currentChat_)
    {
        currentChat_ = chat;

        setCurrentApi(currentChat_->getCurrentApi());
        setCurrentModel(currentChat_->getCurrentModel());

        emit currentChatChanged();
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
            stopGeneration();

            switchToChat(index);
        }

        checkChatsProcessingFinished();

        chatToRemove->deleteLater();

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

    qDebug() << "ChatController::sendMessage ...";

    LLMService* service = llmServices_->get(currentChat_->getCurrentApi());
    if (service)
    {
        if (service->getState() != isWaiting)
        {
            qDebug() << "ChatController::sendMessage ... service not ready !";
            return;
        }
        else
            qDebug() << "ChatController::sendMessage ... service is ready !";

        qDebug() << "ChatController::sendMessage ... start loading spinner";
        emit loadingStarted();

        qDebug() << "ChatController::sendMessage ... post ... prompt: " << text;

        // Add assets (images, audio)
        currentChat_->setAssets(pendingAssets_);
        llmServices_->post(service, currentChat_, text, true);
        clearAssets();
    }
}

void ChatController::stopGeneration()
{
    if (currentChat_)
        llmServices_->stop(currentChat_);
}

int ChatController::currentModelIndex() const
{
    LLMService* currentApi = llmServices_->get(currentAPI_);
    std::vector<LLMModel> models = llmServices_->getAvailableModels(currentApi);
    for (int i = 0; i < models.size(); i++) 
        if (models[i].toString() == currentModel_)
            return i;
    return -1;
}

void ChatController::setCurrentModel(const QString& modelname)
{
    QString currentModel;

    if (!modelname.isEmpty())
        currentModel = modelname;
    else if (currentChat_)
        currentModel = currentChat_->getCurrentModel();
    else
        currentModel = currentModel_;

    if (currentModel != currentModel_)
    {
        qDebug() << "ChatController::setCurrentModel:" << currentModel;
        currentModel_ = currentModel;
        emit currentModelChanged();
    }
}

QVariantList ChatController::getAvailableModels()
{
    QVariantList models;

    qDebug() << "ChatController::getAvailableModels: " << currentAPI_;

    // Get models from current API or all APIs
    LLMService* currentApi = llmServices_->get(currentAPI_);

    const std::vector<LLMModel>& modelList = llmServices_->getAvailableModels(currentApi);
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

void ChatController::setModel(const QString& modelName)
{
    if (currentChat_)
    {
        qDebug() << "ChatController::setModel" << modelName;

        QString currenModelName = currentChat_->getCurrentModel();
        currentChat_->setModel(modelName);
        if (currenModelName != modelName)
        {
            emit currentChatChanged(); // Notify to update UI
            emit chatListChanged();
        }

        connectAPIsSignals();
    }
}

void ChatController::deleteModel(int index)
{
    LLMService* currentApi = currentChat_ ? llmServices_->get(currentChat_->getCurrentApi()) : nullptr;
    const std::vector<LLMModel>& modelList = llmServices_->getAvailableModels(currentApi);
    
    if (index >= modelList.size())
        return;

    const LLMModel& model = modelList[index];

    QString modelName = model.toString();
    if (currentModel_ == modelName)
        setCurrentModel(modelList.front().toString());

    currentApi->deleteModel(model);
    
    qDebug() << "ChatController::deleteModel: index: " << index << modelName;

    emit availableModelsChanged();
}

QVariantList ChatController::getAvailableAPIs() const
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

QVariantMap ChatController::getAPI(const QString& apiname) const
{
    const std::vector<LLMService*>& apiList = llmServices_->getAPIs();
    for (LLMService* api : apiList)
    {
        if (api->name_ == apiname)
            return api->params_;        
    }
    return QVariantMap();
}

int ChatController::currentApiIndex() const
{
    const std::vector<LLMService*>& apiList = llmServices_->getAPIs();
    for (int i=0; i < apiList.size(); i++)
    {
        if (apiList[i]->name_ == currentAPI_)
            return i;        
    }    
    return 0;
}

QVariantList ChatController::getRegisteredAPITypes() const
{
    QVariantList apiTypes;
    QStringList types = { "LlamaCpp", "Ollama" };

    for (const auto& type : types)
    {
        QVariantMap typeInfo;
        typeInfo["name"] = type;
        apiTypes.append(typeInfo);
    }
    
    return apiTypes;
}

void ChatController::setCurrentApi(const QString& apiname)
{
    QString currentApi;

    if (!apiname.isEmpty())
        currentApi = apiname;
    else if (currentChat_)
        currentApi = currentChat_->getCurrentApi();
    
    if (currentApi != currentAPI_)
    {
        if (currentApi.isEmpty())
            currentApi = currentAPI_;

        currentAPI_ = currentApi;
        emit availableModelsChanged(); // Notify that available models list has changed
        
        qDebug() << "ChatController::setCurrentApi:" << currentAPI_;

        emit currentApiChanged();
    }
}

void ChatController::setAPI(const QString& apiName)
{
    if (!currentChat_)
        return;
    
    qDebug() << "ChatController::setAPI" << apiName;

    currentChat_->setApi(apiName);

    emit availableModelsChanged(); // Notify that available models list has changed

    connectAPIsSignals();    
}

bool ChatController::modifyAPI(const QString& apiName, const QString& apiType, const QString& urlstr, const QString& apikey)
{
    qDebug() << "ChatController::modifyAPI:" << apiName << apiType << urlstr;
    
    QUrl url = QUrl::fromUserInput(urlstr);
    if (!url.isValid())
    {
        qWarning() << "ChatController::addAPI: qurl:" << url.toDisplayString() << "not valid !";
        return false;
    }

    QVariantMap params;
    params["name"] = apiName;

    if (apiType == "Ollama")
    {
        params["type"] = static_cast<int>(LLMEnum::LLMType::Ollama);
        params["url"] = url.toDisplayString();
        params["apiver"] = "api/version";
        params["apigen"] = "api/chat";
        params["apikey"] = apikey;
        if (url.host().contains("localhost") || url.host().contains("127.0.0.1"))
        {
            QString ollamaExecutable = QStandardPaths::findExecutable("ollama");
            if (!ollamaExecutable.isEmpty())
            {
                params["executable"] = ollamaExecutable;
                params["programargs"] = QStringList("serve"); 
            }
        }
    }
    
    std::vector<LLMService*> apis = llmServices_->getAPIs();
    auto it = std::find_if(apis.begin(), apis.end(), [apiName](LLMService* api) { return api->name_ == apiName; });
    if (it != apis.end())
    {
        LLMService* api = *it;
        api->params_ = params;
        api->setApiKey(apikey);
    }
    else
    {
        llmServices_->addAPI(LLMService::createService(llmServices_, params));
    }

    emit availableAPIsChanged();

    return true;
}

void ChatController::refreshModels()
{
    qDebug() << "ChatController::refreshModels - Refreshing available models list";

    std::vector<LLMService*> apis = llmServices_->getAPIs();
    for (LLMService* api : apis)    
        api->markAvailableModelsDirty();

    emit availableModelsChanged();
}

void ChatController::resetSettings()
{
    llmServices_->resetSettings();
    ApplicationServices::get<RAGService>()->resetSettings();
    refreshModels();
}

QString ChatController::getChatsFilePath() const
{
    QString dataLocation = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataLocation);
    if (!dir.exists())
        dir.mkpath(".");

    return dir.filePath("chats.json");
}

void ChatController::saveChats(bool sync)
{
    // TODO : sync delta ?

    bool ok = localStore_ && localStore_->save(chats_);
    if (!ok)
        qWarning() << "ChatLocalStore save failed !";
    if (ok && sync)
        cloudStore_->save(chats_);
}

void ChatController::loadChats()
{
    // TODO : sync delta ?

    chats_.clear();

    if (localStore_)
        localStore_->load(chats_);
    
    qDebug() << "ChatController loadChats:" << chats_.size();

    if (!chats_.isEmpty())
    {
        // Connect signals
        for (Chat* chat : chats_)
            QObject::connect(chat, &Chat::processingFinished, this, &ChatController::notifyUpdatedChat);        

        chatCounter_ = chats_.size();
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

QString toBase64(const QString& filePath, const QString& mimeType)
{
    QFile file(filePath);
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
        return QString();
    
    QByteArray fileData = file.readAll();
    file.close();
    
    return QString("data:%1;base64,%2")
        .arg(mimeType)
        .arg(QString::fromLatin1(fileData.toBase64()));
}

void ChatController::addAssetBase64(const QString& assetContent)
{
    if (assetContent.isEmpty())
        return;

    qDebug() << "ChatController::addImageBase64Asset: " << assetContent;
    
    QVariantMap asset;
    asset["type"] = "image";
    asset["base64"] = assetContent;
    asset["path"] = "";
    asset["name"] = "Image collée";
    pendingAssets_.append(asset);

    emit pendingAssetsChanged();
}

void ChatController::addFileAsset(const QString& filePath)
{
    if (filePath.isEmpty())
        return;

    QUrl url(filePath);
    if (!url.isLocalFile())
    {
        qDebug() << "ChatController::addFileAsset: is not localfile " << filePath;
        return;
    }

    qDebug() << "ChatController::addAsset:" << filePath;

    QString localFile = url.toLocalFile();
    QString mimeType = QMimeDatabase().mimeTypeForFile(localFile).name();
    // Si image : convertir en base64
    QString base64;
    if (mimeType.startsWith("image"))
    {
        base64 = toBase64(localFile, mimeType);
        if (base64.isEmpty())
        {
            qWarning() << "Impossible de convertir l'image en base64:" << localFile;
            return;
        }
    }

    // Ajouter à la liste temporaire
    QVariantMap asset;
    asset["type"] = mimeType;
    asset["base64"] = base64;
    asset["path"] = localFile;
    asset["name"] = QFileInfo(localFile).fileName();
    pendingAssets_.append(asset);
}

QString ChatController::getMimeTypeIconFor(const QString &fileName) const
{
    return QString("image://icon/%1").arg(QMimeDatabase().mimeTypeForFile(fileName).iconName());
}

void ChatController::addAssets(const QStringList& urls)
{
    auto oldsize = pendingAssets_.size();

    for (const QString& url : urls)
        addFileAsset(url);

    if (oldsize != pendingAssets_.size())
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
