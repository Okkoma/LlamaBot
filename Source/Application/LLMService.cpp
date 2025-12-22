#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTextBrowser>

#include <QNetworkAccessManager>
#include <qvariant.h>

#include "Chat.h"

#include "LLMService.h"


LLMService::LLMService(QObject* parent) : QObject(parent), widget_(nullptr), allowSharedModels_(false)
{
    initialize();
}

LLMService::~LLMService()
{
    for (LLMAPIEntry* api : apiEntries_)
    {
        if (api)
            delete api;
    }
}

void LLMService::allowSharedModels(bool enable)
{
    allowSharedModels_ = enable;
}

void LLMService::setWidget(QWidget* widget)
{
    widget_ = widget;
}

bool LLMService::loadServiceJsonFile()
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
            addAPI(LLMAPIEntry::fromJson(this, value.toObject()));
    }

    return true;
}

bool LLMService::saveServiceJsonFile()
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
    for (LLMAPIEntry* entry : apiEntries_)
        array << entry->toJson();

    QJsonDocument doc;
    doc.setArray(array);
    QByteArray data = doc.toJson();
    file.write(data);

    return true;
}

bool LLMService::isServiceAvailable(LLMEnum::LLMType service) const
{
    return true;
}

void LLMService::createDefaultServiceJsonFile()
{
    qDebug() << "createDefaultServiceJsonFile ... ";

    QVariantMap params;
    params["lmservice"] = QVariant::fromValue(this);
    params["type"] = static_cast<int>(LLMEnum::LLMType::LlamaCpp);
    params["name"] = "LlamaCpp";
    addAPI(LLMAPIEntry::createService(LLMEnum::LLMType::LlamaCpp, params));

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
        addAPI(LLMAPIEntry::createService(LLMEnum::LLMType::Ollama, params));
    }

    qDebug() << "createDefaultServiceJsonFile ... apis=" << apiEntries_.size();

    saveServiceJsonFile();
}

void LLMService::initialize()
{
    networkManager_ = new QNetworkAccessManager(this);

    if (!loadServiceJsonFile())
        createDefaultServiceJsonFile();

    LLMAPIEntry* defaultService_ = apiEntries_.size() ? apiEntries_.front() : nullptr;

    if (defaultService_ && !defaultService_->isReady())
        defaultService_->start();

    allowSharedModels(true);
}

std::vector<LLMAPIEntry*> LLMService::getAvailableAPIs() const
{
    std::vector<LLMAPIEntry*> results;
    for (LLMAPIEntry* entry : apiEntries_)
    {
        if (entry->isReady())
            results.push_back(entry);
    }
    return results;
}

std::vector<LLMModel> LLMService::getAvailableModels(const LLMAPIEntry* api) const
{
    if (api)
        return api->getAvailableModels();

    // TODO : get all models (all registered apis)
    std::vector<LLMModel> results;
    return results;
}

LLMAPIEntry* LLMService::get(LLMEnum::LLMType service) const
{
    return get(enumValueToString<LLMEnum>("LLMType", service));
}

LLMAPIEntry* LLMService::get(const QString& name) const
{
    for (LLMAPIEntry* entry : apiEntries_)
    {
        if (entry->name_ == name)
            return entry;
    }
    return nullptr;
}

LLMModel* LLMService::getModel(const QString& name) const
{
    for (LLMAPIEntry* api : apiEntries_)
    {
        std::vector<LLMModel> models = api->getAvailableModels();
        for (LLMModel& model : models)
        {
            if (model.toString() == name)
                return &model;
        }
    }
    return nullptr;
}

void LLMService::addAPI(LLMAPIEntry* api)
{
    if (api)
        apiEntries_.push_back(api);
}

void LLMService::post(LLMAPIEntry* api, Chat* chat, const QString& content, bool streamed)
{
    api->post(chat, content, streamed);
}

void LLMService::receive(LLMAPIEntry* api, Chat* chat, const QByteArray& data)
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

bool LLMService::requireStartProcess(LLMAPIEntry* api)
{
    qDebug() << "Require the user authorization for starting the service" << api->name_;
    // Demander la confirmation à l'utilisateur
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(widget_, "Confirmation", "Do you want to start the service " + api->name_ + "?",
        QMessageBox::Yes | QMessageBox::No);
    return (reply == QMessageBox::Yes);
}

void LLMService::handleMessageError(Chat* chat, const QString& message)
{
    bool repost = false;
    LLMAPIEntry* entry = get(chat->currentApi_);

    if (entry && entry->handleMessageError(chat, message))
        post(entry, chat, "", true);
}

void LLMService::stopStream(Chat* chat)
{
    LLMAPIEntry* api = get(chat->currentApi_);
    if (api)
        api->stopStream(chat);
}

std::vector<float> LLMService::getEmbedding(const QString& text)
{
    // Prefer LlamaCpp
    for (LLMAPIEntry* api : apiEntries_)
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
