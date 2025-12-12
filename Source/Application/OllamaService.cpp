#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include "Chat.h"

#include "OllamaService.h"
#include "LLMService.h"


const QString OllamaService::ollamaSystemDir = "/usr/share/ollama/";
const QString OllamaService::ollamaManifestBaseDir = ".ollama/models/manifests/registry.ollama.ai/library/";
const QString OllamaService::ollamaBlobsBaseDir = ".ollama/models/blobs/";

OllamaManifest OllamaService::getOllamaManifest(const QString& ollamaDir, const QString& model, const QString& num_params)
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
        QStringList dirs = ollamaManifestDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::SortFlag::Name);
        for (const QString& dir : dirs)
        {
            QFileInfoList infos = QDir(ollamaDir + "/" + dir).entryInfoList(QDir::Files | QDir::NoSymLinks, QDir::SortFlag::Name);
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

OllamaService::OllamaService(LLMService* service, const QString& name, const QString& url, const QString& ver, const QString& gen, const QString& apiKey, const QString& programPath,
    const QStringList& programArguments) :
    LLMAPIEntry(service, name, LLMEnum::LLMType::Ollama),
    url_(url),
    api_version_(ver),
    api_generate_(gen),
    apiKey_(apiKey),
    programPath_(programPath),
    programArguments_(programArguments)
{ }

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
                        [](const QList<QSslError> &errors) {
            for (const QSslError &error : errors)
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
    QNetworkRequest request(url_ + api_generate_);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    chat->updateContent(content);

    if (chat->stopButton_)
        chat->stopButton_->setEnabled(true);

    // post the request
    QJsonDocument jsonDoc(chat->jsonObject_);
    QByteArray data = jsonDoc.toJson();
    QNetworkReply* reply = service_->networkManager_->post(request, data);
    qDebug() << jsonDoc;

    // connect signals to receive datas
    if (streamed)
    {
        service_->connect(reply, &QNetworkReply::readyRead, service_, [this, chat, reply]() {
            qDebug() << "OllamaService::postInternal streamed: received data";
            service_->receive(this, chat, reply->readAll());
        });
        service_->connect(reply, &QNetworkReply::finished, service_, [this, chat, reply]() {
            qDebug() << "OllamaService::postInternal streamed: finished";
            if (reply->error() == QNetworkReply::NoError)            
                service_->receive(this, chat, reply->readAll());            
            else            
                qDebug() << "Error:" << reply->errorString();                  
            reply->deleteLater();
            if (chat->stopButton_)
                chat->stopButton_->setEnabled(false);
        });
    }
    else
    {
        service_->connect(service_->networkManager_, &QNetworkAccessManager::finished, service_, [this, chat, reply]() {
            if (reply->error() == QNetworkReply::NoError)            
                service_->receive(this, chat, reply->readAll());            
            else            
                qDebug() << "Error:" << reply->errorString();            
            reply->deleteLater();
            if (chat->stopButton_)
                chat->stopButton_->setEnabled(false);            
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
            
            service_->connect(programProcess_.get(), &QProcess::started, service_, [this, chat, content, streamed]() {
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
                    if (QFile::link(ollamaSystemDir + ollamaManifestBaseDir + model + "/" + num_params, userManifestFileName))
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
    for (const QString &arg : programArguments_)
        argsArray.append(arg);
    obj["args"] = argsArray;
    return obj;
}
