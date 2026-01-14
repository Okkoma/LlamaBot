#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

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

    if (llmservices_->hasSharedModels())
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

OllamaService::OllamaService(LLMServices* service, const QString& name, const QString& url, const QString& ver,
    const QString& gen, const QString& apiKey, const QString& programPath, const QStringList& programArguments) :
    LLMService(LLMEnum::LLMType::Ollama, service, name),
    url_(url),
    api_version_(ver),
    api_generate_(gen),
    apiKey_(apiKey),
    programPath_(programPath),
    programArguments_(programArguments)
{
}

OllamaService::OllamaService(LLMServices* service, const QVariantMap& params) :
    LLMService(service, params),
    url_(params["url"].toString()),
    api_version_(params["apiver"].toString()),
    api_generate_(params["apigen"].toString()),
    apiKey_(params["apikey"].toString()),
    programPath_(params["executable"].toString()),
    programArguments_(params["programargs"].toStringList())
{
    networkManager_ = new QNetworkAccessManager(this);
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

    delete networkManager_;

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
        qDebug() << "OllamaService::isUrlAccessible() ..." << url_;

        QEventLoop loop;
        QNetworkReply* reply = networkManager_->get(QNetworkRequest(url_ + api_version_));

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
    QJsonObject payload = chat->getInfo();

    if (useChatApi)
    {
        // Construct messages array for /api/chat
        QJsonArray messagesArray;
        QList<ChatMessage>& history = chat->getHistory();
        if (chat->getHistory().size() > 1)
        {
            for (QList<ChatMessage>::iterator it = history.begin(); it != history.end()-1; ++it)
            {
                QJsonObject msgObj;
                msgObj["role"] = it->role_;
                msgObj["content"] = it->content_;
                messagesArray.append(msgObj);
            }
        }

        // add the last message
        ChatMessage& lastMsg = history.last();
        QJsonObject msgObj;
        msgObj["role"] = lastMsg.role_;
        msgObj["content"] = lastMsg.content_;

        // add the assets of the last message
        const QVariantList& assets = chat->getAssets();
        if (assets.size() > 0)
        {
            QJsonArray imagesArray;
            for (auto& asset : assets)
            {
                QVariantMap map = asset.toMap();
                if (map["type"].toString() == "image")
                {
                    QString image = map["base64"].toString();
                    imagesArray.append(image.sliced(image.indexOf(";base64,",Qt::CaseInsensitive) + 8));
                }
            }
            if (imagesArray.size() > 0)
                msgObj["images"] = imagesArray;
        }
        messagesArray.append(msgObj);

        payload["messages"] = messagesArray;

        if (payload.contains("prompt"))
        {
            // Remove 'prompt' if present, as it conflicts with 'messages' in some versions
            qWarning() << "old version : prompt removed from payload";
            payload.remove("prompt");
        }

        // Add options including num_ctx
        QJsonObject options;
        options["num_ctx"] = chat->getContextSize();
        payload["options"] = options;
    }
    else
    {
        // Fallback or legacy /api/generate
        // chat->jsonObject_ already contains "prompt" via Chat::updateObject()
    }

    QJsonDocument jsonDoc(payload);
    QByteArray data = jsonDoc.toJson();
    QNetworkReply* reply = networkManager_->post(request, data);
    qDebug() << jsonDoc;

    // connect signals to receive datas
    if (streamed)
    {
        llmservices_->connect(reply, &QNetworkReply::readyRead, llmservices_,
            [this, chat, reply, useChatApi]()
            {
                // qDebug() << "OllamaService::postInternal streamed: received data";
                llmservices_->receive(this, chat, reply->readAll());
            });
        llmservices_->connect(reply, &QNetworkReply::finished, llmservices_,
            [this, chat, reply]()
            {
                qDebug() << "OllamaService::postInternal streamed: finished";
                if (reply->error() == QNetworkReply::NoError)
                    llmservices_->receive(this, chat, reply->readAll());
                else
                    qDebug() << "Error:" << reply->errorString();
                reply->deleteLater();
                if (chat)
                    chat->setProcessing(false);
            });
    }
    else
    {
        llmservices_->connect(networkManager_, &QNetworkAccessManager::finished, llmservices_,
            [this, chat, reply]()
            {
                if (reply->error() == QNetworkReply::NoError)
                    llmservices_->receive(this, chat, reply->readAll());
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

        if (canStartProcess() && requireStartProcess())
        {
            start();

            qDebug() << "OllamaService::post: api launched";

            llmservices_->connect(programProcess_.get(), &QProcess::started, llmservices_,
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

bool OllamaService::requireStartProcess()
{
    qDebug() << "Require the user authorization for starting the service" << name_;
    // Demander la confirmation à l'utilisateur
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(nullptr, "Confirmation", "Do you want to start the service " + name_ + "?",
        QMessageBox::Yes | QMessageBox::No);
    return (reply == QMessageBox::Yes);
}

bool OllamaService::handleMessageError(Chat* chat, const QString& message)
{
    if (message.contains("model") && message.contains("not found") && chat->getCurrentApi() == "Ollama")
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
