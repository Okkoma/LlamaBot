#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QTextBrowser>

#include <QNetworkAccessManager>

#include "Chat.h"

#include "LLMService.h"
#include "LlamaCppService.h"
#include "OllamaService.h"

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

void LLMService::createDefaultServiceJsonFile()
{
    qDebug() << "createDefaultServiceJsonFile";

    apiEntries_.push_back(LlamaCppService::createDefault(this, "LlamaCpp"));
    // TODO: this only works on Linux if Ollama is installed in /usr/local/bin
    // find a way to get the path of the ollama binary on other platforms
    // android ? use internal storage and/or find a way to install ollama on android
    // windows ? use the path of the ollama binary in the PATH environment variable
    // mac ? use the path of the ollama binary in the PATH environment variable
    apiEntries_.push_back(new OllamaService(this, "Ollama", "http://localhost:11434/", "api/version", "api/chat", "",
        "/usr/local/bin/ollama", QStringList("serve")));

    saveServiceJsonFile();
}

void LLMService::initialize()
{
    networkManager_ = new QNetworkAccessManager(this);

    if (!loadServiceJsonFile())
        createDefaultServiceJsonFile();

    LLMAPIEntry* defaultService_ = apiEntries_.front();

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

void LLMService::addAPI(LLMAPIEntry* info)
{
    apiEntries_.push_back(info);
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
                chat->chatView_->setMarkdown(chat->messages_.join("\n\n"));
            }
            else if (obj.contains("message"))
            {
                // Handle /api/chat response format
                QJsonObject messageObj = obj["message"].toObject();
                if (messageObj.contains("content"))
                {
                    chat->updateCurrentAIStream(messageObj["content"].toString());
                    chat->chatView_->setMarkdown(chat->messages_.join("\n\n"));
                }
            }
            else if (obj.contains("error"))
            {
                handleMessageError(chat, obj["error"].toString());
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
