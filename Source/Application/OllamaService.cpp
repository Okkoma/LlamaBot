#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>

#include "Chat.h"

#include "LLMService.h"
#include "OllamaService.h"


const QString ollamaDefaultSystemDir = "/usr/share/ollama/";
const QString OllamaService::ollamaSystemDir = []() {
    // Rechercher le chemin partagé standard (par exemple, /usr/share)
    QStringList dataLocations = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &location : dataLocations) {
        QString path = location + "/ollama/";
        if (QDir(path).exists()) {
            qDebug() << "OllamaService::ollamaSystemDir: " << path;
            return path;
        }
    }

    // Si non trouvé, retourner le chemin par défaut
    return ollamaDefaultSystemDir;
}();

const QString OllamaService::ollamaManifestBaseDir = ".ollama/models/manifests/registry.ollama.ai/library/";
const QString OllamaService::ollamaBlobsBaseDir = ".ollama/models/blobs/";

std::vector<LLMModel> OllamaService::getAvailableModels() const
{
    std::vector<LLMModel> result;

    if (service_->allowSharedModels_)
    {
        OllamaService::getOllamaModels(OllamaService::ollamaSystemDir, result);
        OllamaService::getOllamaModels(QDir::homePath() + "/", result);
    }

    qDebug() << "OllamaService::getAvailableModels: " << result.size() << " models found";

    return result;
}

OllamaManifest OllamaService::getOllamaManifest(
    const QString& ollamaDir, const QString& model, const QString& num_params)
{
    OllamaManifest manifest;
    QFile manifestFile(ollamaDir + "/" + model + "/" + num_params);
    if (manifestFile.open(QIODevice::ReadOnly))
    {
        QByteArray manifestData = manifestFile.readAll();
        manifestFile.close();
        QJsonParseError parseError;
        QJsonDocument manifestDoc = QJsonDocument::fromJson(manifestData, &parseError);
        if (parseError.error == QJsonParseError::NoError && manifestDoc.isObject())
        {
            manifest = OllamaManifest::fromJson(manifestDoc.object());
            manifest.model_ = model;
            manifest.num_params_ = num_params;
        }
    }
    return manifest;
}

std::vector<OllamaManifest> OllamaService::getOllamaManifests(const QString& ollamaDir)
{
    std::vector<OllamaManifest> manifests;
    QDir ollamaManifestDir(ollamaDir);
    if (ollamaManifestDir.exists())
    {
        QStringList dirs =
            ollamaManifestDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::SortFlag::Name);
        for (const QString& dir : dirs)
        {
            QFileInfoList infos =
                QDir(ollamaDir + "/" + dir).entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::SortFlag::Name);
            for (QFileInfo& info : infos)
                manifests.push_back(getOllamaManifest(ollamaDir, dir, info.fileName()));
        }
    }
    return manifests;
}

void OllamaService::getOllamaModels(const QString& ollamaDir, std::vector<LLMModel>& models)
{
    std::vector<OllamaManifest> manifests = getOllamaManifests(ollamaDir + ollamaManifestBaseDir);
    for (const OllamaManifest& manifest : manifests)
    {
        LLMModel model;
        model.name_ = manifest.model_;
        model.num_params_ = manifest.num_params_;
        model.filePath_ = ollamaDir + ollamaBlobsBaseDir + manifest.getModelFileName();
        models.push_back(model);
    }
}

OllamaService::OllamaService(LLMService* service, const QString& name, const QString& url, const QString& ver,
    const QString& gen, const QString& apiKey, const QString& programPath, const QStringList& programArguments) :
    LLMAPIEntry(LLMEnum::LLMType::Ollama, service, name),
    url_(url),
    api_version_(ver),
    api_generate_(gen),
    apiKey_(apiKey),
    programPath_(programPath),
    programArguments_(programArguments)
{
}

OllamaService::OllamaService(const QVariantMap& params) :
    LLMAPIEntry(params),
    url_(params["url"].toString()),
    api_version_(params["apiver"].toString()),
    api_generate_(params["apigen"].toString()),
    apiKey_(params["apikey"].toString()),
    programPath_(params["executable"].toString()),
    programArguments_(params["programargs"].toStringList())
{
}

OllamaService::~OllamaService()
{
    stop();
}

bool OllamaService::start()
{
    if (!canStartProcess())
        return false;

    if (!programProcess_)
        programProcess_ = std::shared_ptr<QProcess>(new QProcess());

    if (isProcessStarted() == false)
    {
        programProcess_->start(programPath_, programArguments_);
        qDebug() << "OllamaService: startProcess:" << name_;
    }

    return isProcessStarted();
}

bool OllamaService::stop()
{
    if (!isProcessStarted())
        return true;

    qDebug() << "OllamaService: stopProcess:" << name_;

    programProcess_->terminate();
    return programProcess_->waitForFinished();
}

bool OllamaService::isReady() const
{
    return (isUrlAccessible() && isAPIAccessible()) || isProcessStarted();
}

bool OllamaService::canStartProcess() const
{
    return !programPath_.isEmpty();
}

bool OllamaService::isProcessStarted() const
{
    return programProcess_ && programProcess_->state() != QProcess::NotRunning;
}

// TODO: remove QEventLoop in this function
bool OllamaService::isUrlAccessible() const
{
    if (!url_.isEmpty())
    {
        qDebug() << "OllamaService::isUrlAccessible() ...";

        QEventLoop loop;
        QNetworkReply* reply = service_->networkManager_->get(QNetworkRequest(url_ + api_version_));

        // Gestion du timeout
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        // Gestion des erreurs SSL
        QObject::connect(reply, QOverload<const QList<QSslError>&>::of(&QNetworkReply::sslErrors),
            [](const QList<QSslError>& errors)
            {
                for (const QSslError& error : errors)
                    qDebug() << "SSL Error:" << error.errorString();
            });

        timer.start(5000); // timeout 5s
        loop.exec();

        bool accessible = false;
        if (timer.isActive())
        {
            timer.stop();
            accessible = (reply->error() == QNetworkReply::NoError);
            if (!accessible)
            {
                qDebug() << " ... Erreur réseau:" << reply->errorString();
                qDebug() << " ... Code HTTP:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
            }
        }
        else
        {
            qDebug() << " ... Timeout lors de l'accès à l'URL";
            reply->abort();
        }

        reply->deleteLater();
        return accessible;
    }
    return false;
}

bool OllamaService::isAPIAccessible() const
{
    if (!apiKey_.isEmpty())
    {
        // TODO: check the authorized access to api
    }
    return true;
}

void OllamaService::postInternal(Chat* chat, const QString& content, bool streamed)
{
    // Use api/chat if available or configured
    bool useChatApi = api_generate_.contains("chat");

    QNetworkRequest request(url_ + api_generate_);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    chat->updateContent(content);

    if (chat)
        chat->setProcessing(true);

    // post the request
    QJsonObject payload = chat->jsonObject_;

    if (useChatApi)
    {
        // Construct messages array for /api/chat
        QJsonArray messagesArray;
        for (const ChatMessage& msg : chat->history_)
        {
            QJsonObject msgObj;
            msgObj["role"] = msg.role;
            msgObj["content"] = msg.content;
            messagesArray.append(msgObj);
        }

        // Add the current user prompt if not already in history (updateContent adds it, but let's be safe)
        // Actually chat->updateContent() calls addContent() which adds to history.

        payload["messages"] = messagesArray;
        payload.remove("prompt"); // Remove 'prompt' if present, as it conflicts with 'messages' in some versions
    }
    else
    {
        // Fallback or legacy /api/generate
        // chat->jsonObject_ already contains "prompt" via Chat::updateObject()
    }

    QJsonDocument jsonDoc(payload);
    QByteArray data = jsonDoc.toJson();
    QNetworkReply* reply = service_->networkManager_->post(request, data);
    qDebug() << jsonDoc;

    // connect signals to receive datas
    if (streamed)
    {
        service_->connect(reply, &QNetworkReply::readyRead, service_,
            [this, chat, reply, useChatApi]()
            {
                // qDebug() << "OllamaService::postInternal streamed: received data";

                // For /api/chat, the response format is different:
                // "message": { "role": "assistant", "content": "..." } instead of "response": "..."
                // We need to handle this in LLMService::receive or handle it here.
                // LLMService::receive expects "response".

                // Let's modify LLMService::receive to handle both or intercept here.
                // Since LLMService::receive is generic, we should probably intercept or ensure LLMService handles it.
                // But wait, LLMService::receive parses lines.
                // If we use /api/chat, the stream chunks look like: { "model": "...", "created_at": "...", "message": {
                // "role": "assistant", "content": "Hello" }, "done": false }

                // LLMService::receive checks for "response". We need it to check for "message.content" too.
                // But LLMService::receive is in LLMService.cpp.

                // Actually, let's keep it simple: pass the raw data to service_->receive,
                // and update LLMService::receive to handle "message" object.

                service_->receive(this, chat, reply->readAll());
            });
        service_->connect(reply, &QNetworkReply::finished, service_,
            [this, chat, reply]()
            {
                qDebug() << "OllamaService::postInternal streamed: finished";
                if (reply->error() == QNetworkReply::NoError)
                    service_->receive(this, chat, reply->readAll());
                else
                    qDebug() << "Error:" << reply->errorString();
                reply->deleteLater();
                if (chat)
                    chat->setProcessing(false);
            });
    }
    else
    {
        service_->connect(service_->networkManager_, &QNetworkAccessManager::finished, service_,
            [this, chat, reply]()
            {
                if (reply->error() == QNetworkReply::NoError)
                    service_->receive(this, chat, reply->readAll());
                else
                    qDebug() << "Error:" << reply->errorString();
                reply->deleteLater();
                if (chat)
                    chat->setProcessing(false);
            });
    }
}

void OllamaService::post(Chat* chat, const QString& content, bool streamed)
{
    // api availability : post when api is ready
    if (!isProcessStarted())
    {
        qDebug() << "OllamaService::post: api not started";

        if (canStartProcess() && service_->requireStartProcess(this))
        {
            start();

            qDebug() << "OllamaService::post: api launched";

            service_->connect(programProcess_.get(), &QProcess::started, service_,
                [this, chat, content, streamed]()
                {
                    qDebug() << "OllamaService::post: api started : state=" << programProcess_->state();
                    programProcess_->waitForReadyRead(3000);
                    this->postInternal(chat, content, streamed);
                });
        }
    }
    else
    {
        qDebug() << "OllamaService::post: api ready";
        postInternal(chat, content, streamed);
    }
}

bool OllamaService::handleMessageError(Chat* chat, const QString& message)
{
    if (message.contains("model") && message.contains("not found") && chat->currentApi_ == "Ollama")
    {
        // Ollama : model not found, try to import the model from ollama shared directory in user land
        const QString homePath = QDir::homePath() + "/";

        int model_istart = message.indexOf("'", 0);
        int model_iend = message.indexOf("'", model_istart + 1);
        int params_istart = message.indexOf(":", model_istart + 1);
        QString model = message.sliced(model_istart + 1, params_istart - model_istart - 1);
        QString num_params = message.sliced(params_istart + 1, model_iend - params_istart - 1);

        qDebug() << "handleMessageError: ollama:" << model << num_params << "not found";

        if (QDir(ollamaSystemDir).exists())
        {
            OllamaManifest manifest = getOllamaManifest(ollamaSystemDir + ollamaManifestBaseDir, model, num_params);
            if (!manifest.model_.isEmpty())
            {
                // create in userland the link to the manifest file
                QString userManifestDir = homePath + ollamaManifestBaseDir + model;
                QDir().mkpath(userManifestDir);

                QString userManifestFileName = userManifestDir + "/" + num_params;
                QFileInfo userManifestInfo(userManifestFileName);
                if (!userManifestInfo.exists())
                {
                    if (QFile::link(
                            ollamaSystemDir + ollamaManifestBaseDir + model + "/" + num_params, userManifestFileName))
                        qDebug() << "Lien symbolique manifest créé:" << userManifestFileName;
                    else
                        qWarning() << "Échec de la création du lien manifest:" << userManifestFileName;
                }

                // create in userland the link to the blobs files
                QDir().mkpath(homePath + ollamaBlobsBaseDir);
                QStringList digests;
                digests += manifest.config.digest_;
                for (OllamaManifest::Layer& layer : manifest.layers_)
                    digests += layer.digest_;
                for (QString& digest : digests)
                {
                    QString userBlobPath = homePath + ollamaBlobsBaseDir + digest.replace(':', '-');
                    if (!QFileInfo(homePath + ollamaBlobsBaseDir + digest).exists())
                    {
                        if (QFile::link(ollamaSystemDir + ollamaBlobsBaseDir + digest, userBlobPath))
                            qDebug() << "Lien symbolique blob créé:" << userBlobPath;
                        else
                            qWarning() << "Échec de la création du lien blob:" << userBlobPath;
                    }
                }

                return true;
            }
        }
    }

    return false;
}

QJsonObject OllamaService::toJson() const
{
    QJsonObject obj;
    obj["type"] = enumValueToString<LLMEnum>("LLMType", type_);
    obj["name"] = name_;
    obj["url"] = url_;
    obj["apiver"] = api_version_;
    obj["apigen"] = api_generate_;
    obj["apikey"] = apiKey_; // TODO : encode ?
    obj["executable"] = programPath_;
    QJsonArray argsArray;
    for (const QString& arg : programArguments_)
        argsArray.append(arg);
    obj["args"] = argsArray;
    return obj;
}
