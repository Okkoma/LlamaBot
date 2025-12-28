#include "LLMServices.h"

LLMServices::LLMServices(QObject* parent) : 
    QObject(parent), 
    allowSharedModels_(false)
{
    initialize();
}

LLMServices::~LLMServices()
{
    for (LLMService* api : apiEntries_)
    {
        if (api)
            delete api;
    }
}

void LLMServices::allowSharedModels(bool enable)
{ 
    allowSharedModels_ = enable;
}

void LLMServices::addAPI(LLMService* api)
{
    if (api)
        apiEntries_.push_back(api);        
}

void LLMServices::post(LLMService* api, Chat* chat, const QString& content, bool streamed)
{
    api->post(chat, content, streamed);
}

void LLMServices::receive(LLMService* api, Chat* chat, const QByteArray& data)
{
    QList<QByteArray> lines = data.split('\n');
    for (QByteArray& line : lines)
    {
        if (line.trimmed().isEmpty())
            continue;

        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (doc.isObject())
        {
            QJsonObject obj = doc.object();
            if (obj.contains("response"))
            {
                // Met à jour le dernier message IA
                chat->updateCurrentAIStream(obj["response"].toString());
            }
            else if (obj.contains("message"))
            {
                // Handle /api/chat response format
                QJsonObject messageObj = obj["message"].toObject();
                if (messageObj.contains("content"))
                {
                    chat->updateCurrentAIStream(messageObj["content"].toString());
                }
            }
            else if (obj.contains("error"))
            {
                handleMessageError(chat, obj["error"].toString());
            }
            else
            {
                qWarning() << "Unknown response format : " << obj;
            }
        }
    }
}

void LLMServices::stop(Chat* chat)
{
    LLMService* api = get(chat->getCurrentApi());
    if (api)
        api->stopStream(chat);
}

bool LLMServices::isServiceAvailable(LLMEnum::LLMType service) const 
{ 
    LLMService* api = get(service);
    return api ? api->isReady() : false;
}

LLMService* LLMServices::get(LLMEnum::LLMType service) const
{
    return get(enumValueToString<LLMEnum>("LLMType", service));
}

LLMService* LLMServices::get(const QString& name) const
{
    for (LLMService* entry : apiEntries_)
        if (entry->name_ == name)
            return entry;
    return nullptr;        
}

const std::vector<LLMService*>& LLMServices::getAPIs() const
{ 
    return apiEntries_;
}

std::vector<LLMService*> LLMServices::getAvailableAPIs() const 
{
    std::vector<LLMService*> results;
    for (LLMService* entry : apiEntries_)
    {
        if (entry->isReady())
            results.push_back(entry);
    }
    return results;
}    

std::vector<LLMModel> LLMServices::getAvailableModels(const LLMService* api)
{
    if (api)
        return api->getAvailableModels();
    return {}; // TODO : get all models (all registered apis)   
}

LLMModel LLMServices::getModel(const QString& name) const
{
    for (LLMService* api : apiEntries_)
    {
        std::vector<LLMModel> models = api->getAvailableModels();
        for (const LLMModel& model : models)
        {
            if (model.toString() == name)
                return model;
        }
    }
    return LLMModel();
}

std::vector<float> LLMServices::getEmbedding(const QString& text)
{
    // Prefer LlamaCpp
    for (LLMService* api : apiEntries_)
    {
        if (api->type_ == LLMEnum::LLMType::LlamaCpp && api->isReady())
        {
            std::vector<float> res = api->getEmbedding(text);
            if (!res.empty())
                return res;
        }
    }
    return {};
}

bool LLMServices::loadServiceJsonFile()
{
    QFile file("LLMService.json");
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Unable to open the LLM services configuration file.";
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray())
    {
        qWarning() << "The configuration file must contain a JSON array.";
        return false;
    }

    QJsonArray array = doc.array();
    if (!array.empty())
    {
        for (const QJsonValue& value : array)
            addAPI(LLMService::fromJson(this, value.toObject()));
    }

    qDebug() << "loadServiceJsonFile ... " << file.fileName();

    return true;
}

bool LLMServices::saveServiceJsonFile()
{
    if (!apiEntries_.size())
        return false;

    QFile file("LLMService.json");
    if (!file.open(QIODevice::ReadWrite))
    {
        qWarning() << "Unable to open the LLM services configuration file for writing.";
        return false;
    }

    QJsonArray array;
    for (LLMService* entry : apiEntries_)
        array << entry->toJson();

    QJsonDocument doc;
    doc.setArray(array);
    QByteArray data = doc.toJson();
    file.write(data);

    qDebug() << "saveServiceJsonFile ... " << file.fileName();

    return true;
}

void LLMServices::createDefaultServiceJsonFile()
{
    qDebug() << "createDefaultServiceJsonFile ... ";

    QVariantMap params;
    params["type"] = static_cast<int>(LLMEnum::LLMType::LlamaCpp);
    params["name"] = "LlamaCpp";
    addAPI(LLMService::createService(this, params));

    // Utiliser QStandardPaths pour trouver l'exécutable Ollama
    QString ollamaExecutable = QStandardPaths::findExecutable("ollama");
    if (!ollamaExecutable.isEmpty())
    {
        qDebug() << "Ollama executable found at:" << ollamaExecutable;
        params["type"] = static_cast<int>(LLMEnum::LLMType::Ollama);
        params["name"] = "Ollama";
        params["url"] = "http://localhost:11434/";
        params["apiver"] = "api/version";
        params["apigen"] = "api/chat";
        params["apikey"] = "";
        params["executable"] = ollamaExecutable;
        params["programargs"] = QStringList("serve");
        addAPI(LLMService::createService(this, params));
    }

    qDebug() << "createDefaultServiceJsonFile ... apis=" << apiEntries_.size();

    saveServiceJsonFile();
}

void LLMServices::initialize()
{  
    if (!loadServiceJsonFile())
        createDefaultServiceJsonFile();

    LLMService* defaultService_ = apiEntries_.size() ? apiEntries_.front() : nullptr;

    if (defaultService_ && !defaultService_->isReady())
        defaultService_->start();

    allowSharedModels(true);
}

void LLMServices::handleMessageError(Chat* chat, const QString& message)
{
    bool repost = false;
    LLMService* entry = get(chat->getCurrentApi());

    if (entry && entry->handleMessageError(chat, message))
        post(entry, chat, "", true);
}
