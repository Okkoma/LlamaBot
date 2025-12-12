#include <QDebug>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>

#include <QPushButton>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QMutex>
#include <QWaitCondition>
#include <atomic>

#include "Chat.h"

#include "LlamaCppService.h"
#include "LLMService.h"


std::vector<llama_token> LlamaTokenize(LlamaCppChatData& data, const QString& prompt)
{
    const llama_vocab* vocab = llama_model_get_vocab(data.model_->model_);
    const bool is_first = llama_memory_seq_pos_max(llama_get_memory(data.ctx_), 0) == -1;

    // tokenize the prompt
    const int n_prompt_tokens = -llama_tokenize(vocab, qPrintable(prompt), prompt.size(), NULL, 0, is_first, true);
    std::vector<llama_token> prompt_tokens(n_prompt_tokens);
    if (llama_tokenize(vocab, qPrintable(prompt), prompt.size(), prompt_tokens.data(), prompt_tokens.size(), is_first, true) < 0)
        qDebug() << "failed to tokenize the prompt";

    return prompt_tokens;
}

int LlamaGenerateStep(LlamaCppChatData& data, std::vector<llama_token>& prompt_tokens, QString& response)
{
    const llama_vocab* vocab = llama_model_get_vocab(data.model_->model_);

    // check if we have enough space in the context to evaluate this batch
    int n_ctx = llama_n_ctx(data.ctx_);
    int n_ctx_used = llama_memory_seq_pos_max(llama_get_memory(data.ctx_), 0) + 1;
    if (n_ctx_used + data.batch_.n_tokens > n_ctx)
    {
        qWarning() << "LlamaGenerateStep: error -1 = ontext exceeded";
        return -1;
    }

    int ret = llama_decode(data.ctx_, data.batch_);
    if (ret != 0)
    {
        qWarning() << "LlamaGenerateStep: error -2 = failed to decode";
        return -2;
    }

    // sample the next token
    data.tokenId_ = llama_sampler_sample(data.smpl_, data.ctx_, -1);

    // is it an end of generation?
    if (llama_vocab_is_eog(vocab, data.tokenId_))
    {
        qDebug() << "LlamaGenerateStep: end of generation";
        return 0;
    }

    // convert the token, add it to the response
    char buf[256];
    int n = llama_token_to_piece(vocab, data.tokenId_, buf, sizeof(buf), 0, true);
    // failed to convert token to piece
    if (n < 0)
    {
        qWarning() << "LlamaGenerateStep: error -3 = failed to convert token to piece";
        return -3;
    }
    
    buf[n] = 0;
    qDebug() << "LlamaGenerateStep: response:" << buf << "tokenid:" << data.tokenId_;
    response = QString(buf);

    return static_cast<int>(data.tokenId_);
}



LlamaCppChatData::~LlamaCppChatData()
{
    deinitialize();
}

void LlamaCppChatData::initialize(LlamaModelData* model)
{
    model_ = model;

    // initialize the context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = n_ctx_;
    ctx_params.n_batch = n_ctx_;

    ctx_ = llama_init_from_model(model_->model_, ctx_params);
    if (!ctx_)
    {
        qDebug() << "llama-cpp error: failed to create the llama_context";
        return;
    }

    // initialize the sampler
    smpl_ = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl_, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(smpl_, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(smpl_, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    
    // initialize the default chat template
    llamaCppChattemplate_ = llama_model_chat_template(model_->model_, /* name */ nullptr);

    qDebug() << "llama_initialize: Modèle chargé avec succès";
}

void LlamaCppChatData::deinitialize()
{
    if (smpl_)
    {
        llama_sampler_free(smpl_);
        smpl_ = nullptr;
    }
    if (ctx_)
    {
        llama_free(ctx_);
        ctx_ = nullptr;
    }

    for (llama_chat_message& msg : llamaCppChatMessages_) 
        free(const_cast<char *>(msg.content));
    llamaCppChatMessages_.clear();
}

struct LlamaCppProcessAsync : public LlamaCppProcess
{
    LlamaCppProcessAsync(LlamaCppChatData* data, LLMService* service) : LlamaCppProcess(0, data, service) { }
    ~LlamaCppProcessAsync() override
    {
        stop();
    }

    void start(Chat* chat, const QString& content, bool streamed) override
    {
        // Préparation des tokens et batch pour la génération asynchrone
        data_->chat_ = chat;
        data_->tokens_ = LlamaTokenize(*data_, chat->rawMessages_);
        data_->tokenId_ = 1; // valeur initiale > 0 pour entrer dans la boucle
        data_->response_.clear();
        data_->batch_ = llama_batch_get_one(data_->tokens_.data(), data_->tokens_.size());

        if (chat->stopButton_)
            chat->stopButton_->setEnabled(true);

        // Lancement de la génération asynchrone
        if (!asyncTimer_)
        {
            asyncTimer_ = new QTimer(service_); // parenté sur service_ pour gestion mémoire
            asyncTimer_->setInterval(100);
            QObject::connect(asyncTimer_, &QTimer::timeout, service_, [this]() {
                generate(); 
            });
        }

        startProcess();
    }

    void stop() override
    {
        stopProcess();
    }

    void startProcess()
    {
        if (asyncTimer_ && !asyncTimer_->isActive())
            asyncTimer_->start();               
    }

    void stopProcess() override
    {
        if (asyncTimer_ && asyncTimer_->isActive())
            asyncTimer_->stop();                
    }

    void generate() override
    {
        if (!data_->chat_)
        {
            stopProcess();
            return;
        }
    
        data_->tokenId_ = LlamaGenerateStep(*data_, data_->tokens_, data_->response_);
        data_->chat_->updateCurrentAIStream(data_->response_);
    
        if (data_->tokenId_ <= 0)
        {
            stopProcess();
    
            if (data_->chat_->stopButton_)
                data_->chat_->stopButton_->setEnabled(false);
    
            data_->chat_ = nullptr;
            data_->tokens_.clear();
            data_->response_.clear();
            return;        
        }
    
        // Préparer le batch pour le prochain token
        data_->batch_ = llama_batch_get_one(&data_->tokenId_, 1);  
    }

    QTimer* asyncTimer_ = nullptr;    
};

struct LlamaCppProcessThread : public LlamaCppProcess
{
    LlamaCppProcessThread(LlamaCppChatData* data, LLMService* service) : LlamaCppProcess(1, data, service) { }
    ~LlamaCppProcessThread() override
    {
        stop();
    }

    void start(Chat* chat, const QString& content, bool streamed) override
    {
        // Préparation des données pour la génération threadée
        QMutexLocker locker(&mutex_);
        data_->chat_ = chat;
        data_->tokens_ = LlamaTokenize(*data_, data_->chat_->rawMessages_);
        data_->tokenId_ = 1; // valeur initiale > 0 pour entrer dans la boucle
        data_->response_.clear();
        stopRequested_ = false;
        locker.unlock();

        if (data_->chat_->stopButton_)
            data_->chat_->stopButton_->setEnabled(true);

        // Créer et configurer le worker thread
        if (!worker_)
        {
            worker_ = new LlamaCppWorker(this);
            
            // Connecter les signaux du worker
            QObject::connect(worker_, &LlamaCppWorker::tokenGenerated, service_, [this, chat](const QString& token) {
                chat->updateCurrentAIStream(token);
            });
            
            QObject::connect(worker_, &LlamaCppWorker::generationFinished, service_, [this, chat]() {
                if (chat->stopButton_)
                    chat->stopButton_->setEnabled(false);
                    
                QMutexLocker locker(&mutex_);
                data_->chat_ = nullptr;
                data_->tokens_.clear();
                data_->response_.clear();
            });
            
            QObject::connect(worker_, &LlamaCppWorker::errorOccurred, service_, [this, chat](const QString& error) {
                qWarning() << "LlamaCppApi thread error:" << error;
                if (chat->stopButton_)
                    chat->stopButton_->setEnabled(false);
            });
        }

        // Démarrer le thread
        startProcess();
    }

    void startProcess()
    {
        if (worker_)
        {
            if (!worker_->thread())
            {
                qDebug() << "LlamaCppProcessThread : worker has no thread !";
                return;
            }
            if (!worker_->thread()->isRunning())
                worker_->thread()->start();
        }             
    }

    void stop() override
    {
        stopProcess();

        if (worker_)
        {
            if (worker_->thread() && worker_->thread()->isRunning())
            {
                worker_->thread()->quit();
                worker_->thread()->wait();
            }
            worker_ = nullptr;
        }
    }

    void stopProcess() override
    {
        if (worker_)
            worker_->stopProcessing();              
    }

    LlamaCppWorker* worker_ = nullptr;

    QMutex mutex_;
    QWaitCondition condition_;
    std::atomic<bool> stopRequested_{false};
    std::atomic<bool> isProcessing_{false};    
};

LlamaCppWorker::LlamaCppWorker(LlamaCppProcess *process)
    : process_(process)
{
    thread_ = new QThread();
    this->moveToThread(thread_);

    connect(thread_, &QThread::started, this, &LlamaCppWorker::processRequest);
    connect(this, &LlamaCppWorker::generationFinished, thread_, &QThread::quit);
    connect(this, &LlamaCppWorker::errorOccurred, thread_, &QThread::quit);
    connect(thread_, &QThread::finished, this, &LlamaCppWorker::deleteLater);
    connect(thread_, &QThread::finished, thread_, &QThread::deleteLater);
}

LlamaCppWorker::~LlamaCppWorker()
{
    qDebug() << "~LlamaCppWorker()";

    if (thread_ && thread_->isRunning())
    {
        thread_->quit();
        thread_->wait();
        delete thread_;
    }

    if (process_)
        static_cast<LlamaCppProcessThread*>(process_)->worker_ = nullptr;
}

void LlamaCppWorker::processRequest()
{
    LlamaCppProcessThread* process = static_cast<LlamaCppProcessThread*>(process_);
    LlamaCppChatData& data = *process->data_;

    QMutexLocker locker(&process->mutex_);

    if (!data.chat_ || process->stopRequested_.load())
    {
        emit generationFinished();
        return;
    }
    
    process->isProcessing_ = true;
        
    // Préparer le batch pour le premier token
    data.batch_ = llama_batch_get_one(data.tokens_.data(), data.tokens_.size());
        
    while (!process->stopRequested_.load())
    {
        locker.unlock();
            
        // Générer le prochain token
        data.tokenId_ = LlamaGenerateStep(data, data.tokens_, data.response_);
            
        locker.relock();
            
        if (data.tokenId_ <= 0) // Fin de génération
            break;
        
        // Émettre le token généré
        emit tokenGenerated(data.response_);
            
        // Préparer le batch pour le prochain token
        data.batch_ = llama_batch_get_one(&data.tokenId_, 1);
    }
        
    process->isProcessing_ = false;
    emit generationFinished();

}

void LlamaCppWorker::stopProcessing()
{
    if (process_)
        static_cast<LlamaCppProcessThread*>(process_)->stopRequested_ = true;
}


LlamaCppService::LlamaCppService(LLMService* service, const QString& name) :
    LLMAPIEntry(service, name, LLMEnum::LLMType::LlamaCpp)
{
    // load dynamic backends - IMPORTANT pour activer GPU
    ggml_backend_load_all();

    // Afficher les informations sur les backends disponibles
    qDebug() << getBackendInfo();
}

LlamaCppService* LlamaCppService::createDefault(LLMService* service, const QString& name)
{
    LlamaCppService* llamaService = new LlamaCppService(service, name);

    // Configuration GPU par défaut
    llamaService->setDefaultUseGpu(true);
    llamaService->setDefaultGpuLayers(99); // Toutes les couches sur GPU
    llamaService->setDefaultContextSize(4096);

    // Activer la version threadée par défaut
    llamaService->setUseThreadedVersion(true);

    // Afficher les informations sur les backends disponibles
    qDebug() << "=== Configuration LlamaCpp ===";
    qDebug() << "GPU activé:" << llamaService->isUsingGpu();
    qDebug() << "Couches GPU:" << llamaService->getGpuLayers();
    qDebug() << "Taille contexte:" << llamaService->getContextSize();
    qDebug() << "Version threadée:" << llamaService->isUsingThreadedVersion();

    return llamaService;
}

LlamaCppService::~LlamaCppService()
{
    for (LlamaCppChatData& data : datas_)
        clearData(&data);

    for (LlamaModelData& model : models_)
    {
        if (model.model_)
            llama_model_free(model.model_);        
    }
    
    datas_.clear();
    models_.clear();
}

LlamaCppChatData* LlamaCppService::createData(Chat* chat)
{
    LlamaCppChatData& data = datas_[chat];
    data.chat_ = chat;
    return &data;
}

void LlamaCppService::initializeData(LlamaCppChatData* data, LlamaModelData* model)
{
    data->initialize(model);

    if (useThreadedVersion_)
        data->generateProcess_ = new LlamaCppProcessThread(data, service_);
    else
        data->generateProcess_ = new LlamaCppProcessAsync(data, service_);    
}

void LlamaCppService::clearData(LlamaCppChatData* data)
{
    if (data->generateProcess_)
    {
        delete data->generateProcess_;
        data->generateProcess_ = nullptr;
    }
    data->deinitialize();

    data->model_ = nullptr;
}

LlamaModelData* LlamaCppService::addModel(const QString& modelName, int numGpuLayers)
{
    LlamaModelData modelData;

    std::vector<LLMModel> models = getAvailableModels();
    for (LLMModel& model : models)
    {    
        if (model.toString() == modelName)
        {
            qDebug() << "LlamaCppService::addModel: model" << model.toString() << " file:" << model.filePath_;
            modelData.modelPath_ = model.filePath_;
            break;
        }
    }

    // initialize the model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = numGpuLayers;

    qDebug() << "LlamaCppService::addModel: Chargement du modèle avec" << model_params.n_gpu_layers << "couches GPU";

    modelData.model_ = llama_model_load_from_file(qPrintable(modelData.modelPath_), model_params);
    if (!modelData.model_)
    {
        qDebug() << "llama-cpp error: unable to load model" << modelData.modelPath_;
        return nullptr;
    }
    modelData.modelName_ = modelName;
    modelData.n_gpu_layers_ = numGpuLayers;
    modelData.use_gpu_ = numGpuLayers > 0;
    models_[modelName] = modelData;

    return &models_[modelName];
}

void LlamaCppService::setModel(Chat* chat, QString modelName)
{
    LlamaCppChatData* data = getData(chat);
    if (!data)
        data = createData(chat);

    if (modelName.isEmpty())
    {
        if (data->model_)
            modelName = data->model_->modelName_;
        else
            modelName = chat->currentModel_;
    }

    if (!modelName.isEmpty() && (!data->model_ || modelName != data->model_->modelName_))
    {
        clearData(data);

        LlamaModelData* model = getModel(modelName);
        if (!model)
        {
            model = addModel(modelName, 99);
            if (!model)
            {
                qWarning() << "LlamaCppApi::setModel: no model" << modelName;
                return;        
            }            
        }

        initializeData(data, model);
    } 
}

bool LlamaCppService::isReady() const
{
    return true;
}

void LlamaCppService::post(Chat* chat, const QString& content, bool streamed)
{
    setModel(chat);

    LlamaCppChatData* data = getData(chat); 
    if (!data || !data->model_)
    {
        qDebug() << "LlamaCppService::post no data or no model";
        return;
    }
    chat->updateContent(content);
    data->generateProcess_->start(chat, content, streamed);    
}


QString LlamaCppService::formatMessage(Chat* chat, const QString& role, const QString& content)
{
    LlamaCppChatData* data = getData(chat); 
    if (!data)
    {
        qDebug() << "LlamaCppService::formatMessage: no data";
        return content;
    }

    std::vector<char> formatted(llama_n_ctx(data->ctx_));
    
    if (role == "user")
    {
        // ajouter le message de l'utilisateur
        data->llamaCppChatMessages_.push_back({ "user", content.isEmpty() ? "" : strdup(content.toUtf8().constData())});
    }
    else 
    {
        // ajouter la réponse de l'assistant
        data->llamaCppChatMessages_.push_back({ "assistant", content.isEmpty() ? "" : strdup(content.toUtf8().constData())});
    }

    int new_len = llama_chat_apply_template(data->llamaCppChattemplate_, data->llamaCppChatMessages_.data(), data->llamaCppChatMessages_.size(), true, formatted.data(), formatted.size());
    if (new_len > (int)formatted.size()) 
    {
        formatted.resize(new_len);
        new_len = llama_chat_apply_template(data->llamaCppChattemplate_, data->llamaCppChatMessages_.data(), data->llamaCppChatMessages_.size(), true, formatted.data(), formatted.size());
    }

    QString result = QString::fromLocal8Bit(formatted.data(), new_len);
    qDebug() << "LlamaCppApi::formatMessage: role:" << role << "content:" << content << "result:" << result; 
    return result;
}

void LlamaCppService::stopStream(Chat* chat)
{
    LlamaCppChatData* data = getData(chat);    
    if (data && data->generateProcess_)
        data->generateProcess_->stopProcess();
}

QJsonObject LlamaCppService::toJson() const 
{
    QJsonObject obj;
    obj["type"] = enumValueToString<LLMEnum>("LLMType", type_);
    obj["name"] = name_;
    return obj;
}

// Méthodes de configuration GPU
void LlamaCppService::setDefaultGpuLayers(int n_gpu_layers)
{
    defaultGpuLayers_ = n_gpu_layers;
    qDebug() << "LlamaCppService: Default GPU layers set to" << defaultGpuLayers_;
}

void LlamaCppService::setDefaultContextSize(int n_ctx)
{
    defaultContextSize_ = n_ctx;
    qDebug() << "LlamaCppService: Default Context size set to" << defaultContextSize_;
}

void LlamaCppService::setDefaultUseGpu(bool use_gpu)
{
    defaultUseGpu_ = use_gpu;
    qDebug() << "LlamaCppService: Default GPU usage set to" << defaultUseGpu_;
}

QStringList LlamaCppService::getAvailableBackends()
{
    QStringList backends;
        
    for (size_t i = 0; i < ggml_backend_reg_count(); i++)
    {
        ggml_backend_reg_t reg = ggml_backend_reg_get(i);
        if (reg)
        {
            QString backendName = QString::fromUtf8(ggml_backend_reg_name(reg));
            backends.append(backendName);
        }
    }
    
    return backends;
}

QString LlamaCppService::getBackendInfo()
{
    QString info;
    info += "=== Backends disponibles ===\n";
    
    QStringList backends = getAvailableBackends();
    for (const QString& backend : backends)
    {
        info += "- " + backend + "\n";
    }
    
    info += "\n=== Devices disponibles ===\n";
    for (size_t i = 0; i < ggml_backend_dev_count(); i++)
    {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        if (dev)
        {
            QString devName = QString::fromUtf8(ggml_backend_dev_name(dev));
            QString devDesc = QString::fromUtf8(ggml_backend_dev_description(dev));
            info += QString("- %1: %2\n").arg(devName, devDesc);
        }
    }
    
    return info;
}

int LlamaCppService::getGpuLayers(Chat* chat) const
{
    const LlamaCppChatData* data = chat ? getData(chat) : nullptr;
    if (data && data->model_)
        return data->model_->n_gpu_layers_;
    return defaultGpuLayers_;
}

bool LlamaCppService::isUsingGpu(Chat* chat) const
{ 
    const LlamaCppChatData* data = chat ? getData(chat) : nullptr;
    if (data && data->model_)
        return data->model_->use_gpu_;    
    return defaultUseGpu_;
}

int LlamaCppService::getContextSize(Chat* chat) const
{
    const LlamaCppChatData* data = chat ? getData(chat) : nullptr;    
    return data ? data->n_ctx_ : defaultContextSize_;
}

LlamaModelData* LlamaCppService::getModel(const QString& modelname)
{
    QHash<QString, LlamaModelData >::iterator it = models_.find(modelname);
    return it != models_.end() ? &it.value() : nullptr;
}

const LlamaModelData* LlamaCppService::getModel(const QString& modelname) const
{
    QHash<QString, LlamaModelData >::const_iterator it = models_.find(modelname);
    return it != models_.end() ? &it.value() : nullptr;
}

LlamaCppChatData* LlamaCppService::getData(Chat* chat)
{
    QHash<Chat*, LlamaCppChatData>::iterator it = datas_.find(chat);
    return it != datas_.end() ? &it.value() : nullptr;
}

const LlamaCppChatData* LlamaCppService::getData(Chat* chat) const
{
    QHash<Chat*, LlamaCppChatData>::const_iterator it = datas_.find(chat);
    return it != datas_.end() ? &it.value() : nullptr;
}
