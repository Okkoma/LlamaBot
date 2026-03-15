#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

#include <QtConcurrent/QtConcurrent>

#include "ApplicationServices.h"
#include "ErrorSystem.h"

#include "RAGService.h"
#include "OllamaService.h"

#include "LlamaCppService.h"

static int ERRCODE_LLAMACPP_ONTEXT_EXCEEDED = ErrorSystem::instance().registerError("LlamaCpp - ontext exceeded");
static int ERRCODE_LLAMACPP_FAILED_DECODE   = ErrorSystem::instance().registerError("LlamaCpp - failed to decode");
static int ERRCODE_LLAMACPP_FAILED_CONVERT_TOKEN_TO_PIECE = ErrorSystem::instance().registerError("LlamaCpp - failed to convert token to piece");
static int ERRCODE_LLAMACPP_FAILED_CREATE_CONTEXT = ErrorSystem::instance().registerError("LlamaCpp - failed to create context");
static int ERRCODE_LLAMACPP_FAILED_LOAD_MODEL = ErrorSystem::instance().registerError("LlamaCpp - failed to load model");

int generationErrorCodes_[]
{
    0,
    ERRCODE_LLAMACPP_ONTEXT_EXCEEDED,
    ERRCODE_LLAMACPP_FAILED_DECODE,
    ERRCODE_LLAMACPP_FAILED_CONVERT_TOKEN_TO_PIECE
};

// Utility function to check available GPU memory
bool checkGpuMemoryAvailable(size_t requiredBytes=0, bool log=false)
{
    size_t freeMem = 0;
    size_t totalMem = 0;
    bool hasEnoughMemory = false;
    
    // Check all available GPU devices
    if (log && requiredBytes)
        qDebug() << QString("GPU memory check - Required: %1 MiB").arg(requiredBytes / (1024 * 1024));
    
    for (size_t i = 0; i < ggml_backend_dev_count(); i++)
    {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        if (dev)
        {
            ggml_backend_dev_memory(dev, &freeMem, &totalMem);
            QString devName = QString::fromUtf8(ggml_backend_dev_name(dev));
            QString devDesc = QString::fromUtf8(ggml_backend_dev_description(dev));
            if (log)
                qDebug() << QString("Device %1 (%2): %3 MiB free / %4 MiB total")
                            .arg(devName)
                            .arg(devDesc)
                            .arg(freeMem / (1024 * 1024))
                            .arg(totalMem / (1024 * 1024));

            if (devName.contains("BLAS"))
                continue;
            
            if (requiredBytes)
            {
                if (freeMem >= requiredBytes)
                {
                    hasEnoughMemory = true;
                    if (log)
                        qDebug() << QString("- Sufficient memory on device %1").arg(devName);
                }
                else
                {
                    qWarning() << QString("! Insufficient memory on device %1 (missing %2 MiB)")
                                .arg(devName)
                                .arg((requiredBytes - freeMem) / (1024 * 1024));
                }
            }
        }
    }
    
    return hasEnoughMemory || !requiredBytes;
}

// Function to wait for GPU memory to be released
// CUDA operations are asynchronous, so after llama_free(), we need to wait
// for the CUDA driver to actually release the memory before we can reallocate
void waitForGpuMemoryPurge()
{
    qDebug() << "Waiting for GPU memory to be released...";
    
    // Iterate through all available devices (for logging)
    for (size_t i = 0; i < ggml_backend_dev_count(); i++)
    {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        if (dev)
        {
            QString devName = QString::fromUtf8(ggml_backend_dev_name(dev));
            qDebug() << QString("Device %1 waiting for memory release...").arg(devName);
        }
    }
    
    // Wait for CUDA to release memory
    // CUDA operations are asynchronous and memory may take time to be released
    // We wait in multiple steps to allow the system to release progressively
    QThread::msleep(100);
    QThread::msleep(100);
    QThread::msleep(100);
    
    qDebug() << "GPU memory release wait completed";
}

llama_context* LLamaInitializeContext(llama_model* model, const llama_context_params& params, bool log)
{
    llama_context* ctx = nullptr;

    // Estimate required memory (approximate: context + batch + buffers)
    // Based on logs, we need:
    // - KV cache: 256 MiB (allocated first)
    // - Compute buffers: 258.5 MiB (allocated next, fails here)
    // - Total: ~514-520 MiB to be safe
    // We add a 20% safety margin for fragmentation
    size_t kvCacheMemory = 256 * 1024 * 1024; // 256 MiB for KV cache
    size_t computeBuffers = 271056896; // 258.5 MiB for compute buffers
    size_t estimatedMemory = kvCacheMemory + computeBuffers;
    estimatedMemory = static_cast<size_t>(estimatedMemory * 1.2); // +20% safety margin
    
    if (log)
    {
        qDebug() << "LLamaInitializeContext: Creating context with n_ctx=" << params.n_ctx;
        qDebug() << QString("Estimated memory required: %1 MiB (KV cache: 256 MiB + Buffers: 258.5 MiB + 20% margin)")
                    .arg(estimatedMemory / (1024 * 1024));
    }
    
    // Check available memory before creating the context
    if (!checkGpuMemoryAvailable(estimatedMemory, log))
    {
        qWarning() << "Insufficient GPU memory detected, waiting for release...";
        
        // Release all other contexts if possible
        // Note: We cannot directly access the service from here,
        // so we just wait for memory to be released
        waitForGpuMemoryPurge();
                
        // Check again after purge
        if (!checkGpuMemoryAvailable(estimatedMemory, log))
        {
            qWarning() << "llama-cpp error: Insufficient GPU memory even after purge";
            qWarning() << QString("Required: %1 MiB, but insufficient memory available").arg(estimatedMemory / (1024 * 1024));
            qWarning() << "Check that GPU memory is sufficient and no other process is using the GPU";
            qWarning() << "Try closing other applications using the GPU or reduce n_ctx";
            return nullptr;
        }
    }
    
    ctx = llama_init_from_model(model, params);
    if (!ctx)
    {
        qWarning() << "llama-cpp error: failed to create the llama_context - possible GPU memory issue";
        qWarning() << "Waiting for GPU release and rechecking...";
        
        // On failure, wait for memory to be released and retry once
        waitForGpuMemoryPurge();
        
        if (!checkGpuMemoryAvailable(estimatedMemory, log))
        {
            qWarning() << "GPU memory still insufficient after purge";
            return nullptr;
        }
        
        // Retry once after purge
        qDebug() << "Retrying context creation after purge...";
        ctx = llama_init_from_model(model, params);
        if (!ctx)
        {
            qWarning() << "llama-cpp error: Final failure to create context";
            qWarning() << "Check that GPU memory is sufficient and no other process is using the GPU";
            return nullptr;
        }
    }

    return ctx;
}

std::vector<llama_token> LlamaTokenize(llama_model* model, const QString& prompt, bool add_special = true)
{
    const llama_vocab* vocab = llama_model_get_vocab(model);

    // tokenize the prompt
    std::string text = prompt.toStdString();
    const int n_prompt_tokens = -llama_tokenize(vocab, text.c_str(), text.size(), NULL, 0, add_special, true);
    std::vector<llama_token> prompt_tokens(n_prompt_tokens);
    if (llama_tokenize(vocab, text.c_str(), text.size(), prompt_tokens.data(), prompt_tokens.size(), add_special, true) < 0)
    {
        qDebug() << "failed to tokenize the prompt";
        return {};
    }

    return prompt_tokens;
}

std::vector<llama_token> LlamaTokenize(llama_context* ctx, llama_model* model, const QString& prompt)
{
    const bool is_first = llama_memory_seq_pos_max(llama_get_memory(ctx), 0) == -1;
    return LlamaTokenize(model, prompt, is_first);
}

std::vector<llama_token> LlamaTokenize(LlamaCppChatData& data, const QString& prompt)
{
    return LlamaTokenize(data.ctx_, data.model_->model_, prompt);
}

QString LLamaDetokenize(LlamaCppChatData& data, const std::vector<llama_token>& tokens, bool skipLastToken)
{
    // param skipLastToken : generally to remove the "end of sentence" token

    qDebug() << "LLamaDetokenize ...";
    QTime start = QTime::currentTime();
    
    const llama_vocab* vocab = llama_model_get_vocab(data.model_->model_);
    const int size = tokens.size() * LLM_MAX_TOKEN_LEN;
    QVector<char> text1;
    text1.resize(size);
    int n = llama_detokenize(vocab, tokens.data(), tokens.size() + (skipLastToken ? -1 : 0), text1.data(), text1.size() - 1, true, true);
    text1.resize(n);

    qDebug() << "LLamaDetokenize ... size=" << n << " perf=" << (QTime::currentTime().msec() - start.msec()) << "ms";
    return QString(text1.data());
}

int getUsedVRAM(llama_context* ctx) 
{
    return 1 + llama_memory_seq_pos_max(llama_get_memory(ctx), 0) 
             - llama_memory_seq_pos_min(llama_get_memory(ctx), 0);
}

int LlamaGenerateStep(LlamaCppChatData& data)
{
    //qDebug() << "LlamaGenerateStep:";

    const llama_vocab* vocab = llama_model_get_vocab(data.model_->model_);

    // check if we have enough space in the context to evaluate this batch
    data.n_ctx_ = llama_n_ctx(data.ctx_);
    data.n_ctx_used_ = getUsedVRAM(data.ctx_);

    if (data.batch_.n_tokens + data.n_ctx_used_ >= data.n_ctx_ - 50) // add 50 tokens for security
    {
        qWarning() << "LlamaGenerateStep: error -1 = ontext exceeded";
        qDebug()   << "    n_ctx: " << data.n_ctx_
                   << "    tokens in RAM: " << data.batch_.n_tokens
                   << "    tokens in VRAM:" << data.n_ctx_used_;
        return -1;
    }

    int ret = llama_decode(data.ctx_, data.batch_);
    if (ret != 0)
    {
        qWarning() << "LlamaGenerateStep: error -2 = failed to decode";
        return -2;
    }

    // sample the next token
    data.currentToken_ = llama_sampler_sample(data.smpl_, data.ctx_, -1);
    // store the new generated token in response vector
    data.response_tokens_.push_back(data.currentToken_);

    data.n_ctx_used_ = getUsedVRAM(data.ctx_);

    // is it an end of generation?
    if (llama_vocab_is_eog(vocab, data.currentToken_))
    {
        qDebug() << "LlamaGenerateStep: end of generation";
        return 0;
    }

    // convert the token, add it to the response
    char buf[256];
    int n = llama_token_to_piece(vocab, data.currentToken_, buf, sizeof(buf), 0, true);
    // failed to convert token to piece
    if (n < 0)
    {
        qWarning() << "LlamaGenerateStep: error -3 = failed to convert token to piece";
        return -3;
    }

    buf[n] = 0;
    //qDebug() << "LlamaGenerateStep: response:" << buf << "currentToken:" << data.currentToken_;
    data.response_ = QString(buf);

    return static_cast<int>(data.currentToken_);
}


void LlamaCppChatData::initialize(LlamaModelData* model)
{
    model_ = model;

    // check n_ctx
    int n_tokens = context_tokens_.size();
    if (!n_tokens)
    {
        std::vector<llama_token> full_history;
        QString formattedHistory;
        if (chat_)
        {
            formattedHistory = chat_->getFormattedHistory();
            if (!formattedHistory.isEmpty())
                full_history = LlamaTokenize(model_->model_, formattedHistory);
        }
        n_tokens = full_history.size();
        qDebug() << "llama_initialize: retokenized full history size:" << n_tokens;
    }
    
    // enlarge context size if needed
    int new_ctx = n_ctx_;
    while (new_ctx < n_tokens)
        new_ctx *= 2;
    if (new_ctx != n_ctx_)
    {
        n_ctx_ = new_ctx;
        qDebug() << "llama_initialize: enlarge the context size to:" << new_ctx;
    }

    // initialize the context
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = n_ctx_;
    //ctx_params.n_batch = n_ctx_ > LLM_BATCH_SIZE ? LLM_BATCH_SIZE : n_ctx_; // Limit batch size to reasonable value
    ctx_params.n_batch = n_ctx_;
    // TODO: 
    // Add method to detect quantification capabilities for the model
    // Use it to set the quantification type, with Q8_0 as default
    // If large context, use Q4_0
    // KV cache quantification
    ctx_params.type_k = GGML_TYPE_Q8_0;  // Keys Quantification 
    ctx_params.type_v = GGML_TYPE_Q8_0;  // Values Quantification
    ctx_params.flash_attn_type = LLAMA_FLASH_ATTN_TYPE_ENABLED;

    assert(ctx_ == nullptr);

    ctx_ = LLamaInitializeContext(model_->model_, ctx_params, true);
    if (!ctx_)
    {
        ErrorSystem::instance().log(ERRCODE_LLAMACPP_FAILED_CREATE_CONTEXT);
        return;        
    }

    // initialize the sampler
    smpl_ = llama_sampler_chain_init(llama_sampler_chain_default_params());
    llama_sampler_chain_add(smpl_, llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(smpl_, llama_sampler_init_temp(0.8f));
    llama_sampler_chain_add(smpl_, llama_sampler_init_dist(LLAMA_DEFAULT_SEED));

    // initialize the default chat template
    llamaCppChattemplate_ = llama_model_chat_template(model_->model_, /* name */ nullptr);

    qDebug() << "llama_initialize: success";
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
        qDebug() << "LlamaCppChatData::deinitialize: Freeing llama context ... this:" << this << " ctx:" << ctx_;
        // Free the context
        llama_free(ctx_);
        ctx_ = nullptr;
        
        // Wait a bit to allow CUDA to release GPU memory
        // CUDA operations are asynchronous and memory may not be
        // immediately available after llama_free()
        qDebug() << "LlamaCppChatData::deinitialize: Waiting for GPU memory release...";
        waitForGpuMemoryPurge();
        qDebug() << "LlamaCppChatData::deinitialize: Context freed";
    }
}

void LlamaCppChatData::clear()
{
    if (context_tokens_.size())
        qWarning() << "LlamaCppChatData::clear";

    context_tokens_.clear();
    response_tokens_.clear();
    response_.clear();
}

bool prepareStartGeneration(LlamaCppChatData& data, Chat* chat, bool resetted=false);

void LlamaCppChatData::reset()
{
    qDebug() << "LlamaCppChatData::reset";
    deinitialize();
    
    if (model_)
    {
        initialize(model_);
        if (!ctx_)
        {
            qWarning() << "LlamaCppChatData::reset ... no context !";
            return;        
        }
    }
    
    if (response_tokens_.size())
        context_tokens_.insert(context_tokens_.end(), response_tokens_.begin(), response_tokens_.end());

    prepareStartGeneration(*this, chat_, true);
}

bool prepareStartGeneration(LlamaCppChatData& data, Chat* chat, bool resetted)
{
    data.chat_ = chat;
    data.response_.clear();
    if (!resetted)
        data.response_tokens_.clear();
    
    // if a history exists and the tokens are not already got, do it with the full history
    if (chat->getHistory().size() > 2 && !data.context_tokens_.size())
    {
        QString formatedEntry = chat->getFormattedHistory();
        data.context_tokens_ = LlamaTokenize(data, formatedEntry);
        qDebug() << "prepareStartGeneration: tokenize all history";
    }
    
    // add the new user prompt
    if (!resetted)
    {
        QString formatedEntry = chat->getFormattedMessage("user", -1);
        data.prompt_tokens_ = LlamaTokenize(data, formatedEntry);
        data.context_tokens_.insert(data.context_tokens_.end(), data.prompt_tokens_.begin(), data.prompt_tokens_.end());
        qDebug() << "prepareStartGeneration: insert new user message in prompt";
    }
    else
        data.prompt_tokens_.clear();

    bool contextInVRAM = getUsedVRAM(data.ctx_) >= data.context_tokens_.size() - data.prompt_tokens_.size() -1;
    qDebug() << "prepareStartGeneration: getUsedVRAM:" << getUsedVRAM(data.ctx_) << "context_tokens:" << data.context_tokens_.size() - data.prompt_tokens_.size();

    std::vector<llama_token>* entryTokens;
    if (contextInVRAM && !resetted)
    {
        qDebug() << "prepareStartGeneration: context in VRAM, adding only the prompt to the batch ... resetted:" << resetted;
        entryTokens = &data.prompt_tokens_;
    }
    else
    {
        qDebug() << "prepareStartGeneration: no context in VRAM, adding all the context to the batch ... resetted:" << resetted;
        entryTokens = &data.context_tokens_;
    }

    qDebug() << "prepareStartGeneration: entryTokens" << entryTokens->size();
    data.batch_ = llama_batch_get_one(entryTokens->data(), entryTokens->size());
    return true;
}

void setBatchForNextToken(LlamaCppChatData& data)
{
    data.batch_ = llama_batch_get_one(&data.currentToken_, 1);
}

struct LlamaCppProcessAsync : public LlamaCppProcess
{
    LlamaCppProcessAsync(LlamaCppChatData* data, LLMServices* service) : LlamaCppProcess(0, data, service) {}
    ~LlamaCppProcessAsync() override { stop(); }

    void start(Chat* chat, const QString& content, bool streamed) override
    {
        // Prepare tokens and batch for asynchronous generation
        if (!prepareStartGeneration(*data_, chat))
        {
            if (chat)
                chat->setProcessing(false);
            stopProcess();
            return;
        }

        if (chat)
            chat->setProcessing(true);

        // Launch asynchronous generation
        if (!asyncTimer_)
        {
            asyncTimer_ = new QTimer(service_); // parented to service_ for memory management
            asyncTimer_->setInterval(100);
            QObject::connect(asyncTimer_, &QTimer::timeout, service_,
                [this]()
                {
                    generate();
                });
        }

        startProcess();
    }

    void stop() override { stopProcess(); }

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

        data_->currentToken_= LlamaGenerateStep(*data_);

        if (data_->currentToken_ == -1 && data_->chat_ && data_->chat_->getLLMServices()->getAutoExpandContext())
        {
            qDebug() << "LlamaCppProcessAsync: Context auto-expanded. Re-initializing context...";   
            // Auto-expand logic
            int newSize = data_->n_ctx_ * 2;
            int n_ctx_train = llama_model_n_ctx_train(data_->model_->model_);
            if (newSize <= n_ctx_train)
            {
                qDebug() << "LlamaCppProcessAsync: Auto-expanding context to" << newSize;
                data_->chat_->setContextSize(newSize);
                // break to next step
                return;
            }
        }

        data_->chat_->updateCurrentAIStream(data_->response_);
        
        if (data_->currentToken_ <= 0)
        {
            stopProcess();

            if (data_->chat_)
                data_->chat_->setProcessing(false);

            data_->chat_ = nullptr;
            return;
        }

        setBatchForNextToken(*data_);
    }

    QTimer* asyncTimer_ = nullptr;
};

struct LlamaCppProcessThread : public LlamaCppProcess
{
    LlamaCppProcessThread(LlamaCppChatData* data, LLMServices* service) : LlamaCppProcess(1, data, service) {}
    ~LlamaCppProcessThread() override { stop(); }

    void start(Chat* chat, const QString& content, bool streamed) override
    {
        qDebug() << "LlamaCppProcessThread::start()";

        // Prepare data for threaded generation
        QMutexLocker locker(&mutex_);
        if (!prepareStartGeneration(*data_, chat))
        {
            locker.unlock();
            if (chat)
                chat->setProcessing(false);
            stop();
            locker.unlock();
            return;
        }
        stopRequested_ = false;
        locker.unlock();

        if (data_->chat_)
            data_->chat_->setProcessing(true);

        // Create and configure worker thread
        if (!worker_)
        {
            qDebug() << "LlamaCppProcessThread::start() ... create new worker";
            worker_ = new LlamaCppWorker(this);

            // Connect worker signals
            QObject::connect(worker_, &LlamaCppWorker::tokenGenerated, service_,
                [this, chat](const QString& token)
                {
                    chat->updateCurrentAIStream(token);
                });

            QObject::connect(worker_, &LlamaCppWorker::generationFinished, service_,
                [this, chat]()
                {
                    if (chat)
                        chat->setProcessing(false);
                    
                    QMutexLocker locker(&mutex_);
                    data_->chat_ = nullptr;
                    this->stop();
                });

            QObject::connect(worker_, &LlamaCppWorker::errorOccurred, service_,
                [this, chat]()
                {
                    if (chat)
                        chat->setProcessing(false);
                });
        }

        // Start the thread
        startProcess();
    }

    void startProcess()
    {
        if (worker_)
        {
            qDebug() << "LlamaCppProcessThread::startProcess()";
            if (!worker_->thread())
            {
                qDebug() << "LlamaCppProcessThread : worker has no thread !";
                return;
            }
            if (!worker_->thread()->isRunning())            
                worker_->thread()->start();
            else
                qWarning() << "LlamaCppProcessThread::startProcess() : worker is already running!";
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
    std::atomic<bool> stopRequested_{ false };
    std::atomic<bool> isProcessing_{ false };
};

LlamaCppWorker::LlamaCppWorker(LlamaCppProcess* process) : process_(process)
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

    while (!process->stopRequested_.load())
    {
        locker.unlock();

        // Generate next token
        data.currentToken_ = LlamaGenerateStep(data);

        locker.relock();

        if (data.currentToken_ == -1 && data.chat_ && data.chat_->getLLMServices()->getAutoExpandContext())
        {
            qDebug() << "LlamaCppWorker: Context auto-expanded. Re-initializing context...";
            // Auto-expand logic
            int newSize = data.n_ctx_ * 2;
            int n_ctx_train = llama_model_n_ctx_train(data.model_->model_);
            if (newSize <= n_ctx_train)
            {
                qDebug() << "LlamaCppWorker: Auto-expanding context to" << newSize;
                data.chat_->setContextSize(newSize);
                if (!data.ctx_)
                {
                    ErrorSystem::instance().log(generationErrorCodes_[-data.currentToken_]);
                    emit errorOccurred();
                    break;
                }
                // continue to next step
                continue;
            }
        }

        if (data.currentToken_ <= 0) // End of generation
        {
            if (data.currentToken_ < 0)
            {
                ErrorSystem::instance().log(generationErrorCodes_[-data.currentToken_]);
                emit errorOccurred();
            }
            break;
        }

        // Emit generated token
        emit tokenGenerated(data.response_);
        setBatchForNextToken(data);
    }

    QString finalResponse = LLamaDetokenize(data, data.response_tokens_, true);
    qDebug() << "finalResponse:" << finalResponse;

    if (data.response_tokens_.size())
    {
        data.context_tokens_.insert(data.context_tokens_.end(), data.response_tokens_.begin(), data.response_tokens_.end());
        data.response_tokens_.clear();
    }

    //QString allcontext = LLamaDetokenize(data, data.context_tokens_, false);
    //qDebug() << "allcontext:" << allcontext;
    
    emit tokenGenerated(finalResponse + "<end>");
    process->isProcessing_ = false;
    emit generationFinished();
}

void LlamaCppWorker::stopProcessing()
{
    if (process_)
        static_cast<LlamaCppProcessThread*>(process_)->stopRequested_ = true;
}


LlamaCppService::LlamaCppService(LLMServices* service, const QString& name) :
    LLMService(LLMEnum::LLMType::LlamaCpp, service, name)
{
    bool check = checkGpuMemoryAvailable();

    // load dynamic backends - IMPORTANT to enable GPU
    ggml_backend_load_all();

    // Display information about available backends
    qDebug().noquote() << getBackendInfo();
}

const char* GGML_LOG_LEVEL_STRINGS[] =
{
    "GGML_NONE:",
    "GGML_DEBUG:",
    "GGML_INFO:",
    "GGML_WARN:",
    "GGML_ERROR:"
};

void logGGMLtoQT(enum ggml_log_level level, const char * text, void * user_data)
{
    QString msg(text);
    msg.resize(msg.length() - 1);
    if (level < 5)
        qDebug().noquote() << GGML_LOG_LEVEL_STRINGS[level] << msg;
    else
        qDebug().noquote() << msg;
}

void emptyLog(enum ggml_log_level level, const char * text, void * user_data) { ; }

LlamaCppService::LlamaCppService(LLMServices* service, const QVariantMap& params) :
    LLMService(service, params)
{
    //ggml_log_set(&logGGMLtoQT, nullptr);
    llama_log_set(&emptyLog, nullptr);

    // Default GPU configuration
    setDefaultUseGpu(true);
    setDefaultGpuLayers(99); // All layers on GPU
    setDefaultContextSize(LLM_DEFAULT_CONTEXT_SIZE);

    // Enable threaded version by default
    setUseThreadedVersion(true);

    bool check = checkGpuMemoryAvailable();

    // load dynamic backends - IMPORTANT to enable GPU
    ggml_backend_load_all();

    // Display information about available backends
    qDebug().noquote() << getBackendInfo();
        
    // Display information about available backends
    qDebug() << "=== Configuration LlamaCpp ===";
    qDebug() << "GPU activé:" << isUsingGpu();
    qDebug() << "Couches GPU:" << getGpuLayers();
    qDebug() << "Taille contexte:" << getContextSize();
    qDebug() << "Version threadée:" << isUsingThreadedVersion();
}

LlamaCppService::~LlamaCppService()
{
    qDebug() << "~LlamaCppService";

    for (auto data : datas_)
        clearData(&data.second);

    for (auto model : models_)
        if (model.second.model_)
            llama_model_free(model.second.model_);

    checkGpuMemoryAvailable();
}

LlamaCppChatData* LlamaCppService::createData(Chat* chat)
{
    qDebug() << "LlamaCppService::createData ... chat:" << chat;

    LlamaCppChatData& data = datas_[chat];
    data.chat_ = chat;
    chat->setChatData(&data);

    return &data;
}

void LlamaCppService::initializeData(LlamaCppChatData* data, LlamaModelData* model)
{
    qDebug() << "LlamaCppService::initializeData ... data:" << data << " model: " << model;

    if (!model)
    {
        qWarning() << "LlamaCppService::initializeData ... no model !";
        return;        
    }

    data->initialize(model);
    if (!data->ctx_)
    {
        qWarning() << "LlamaCppService::initializeData ... no context !";
        return;        
    }

    if (!data->generateProcess_)
    {
        if (useThreadedVersion_)
            data->generateProcess_ = new LlamaCppProcessThread(data, llmservices_);
        else
            data->generateProcess_ = new LlamaCppProcessAsync(data, llmservices_);
    }    
}

void LlamaCppService::clearData(LlamaCppChatData* data)
{
    qDebug() << "LlamaCppService::clearData: Cleaning up data:" << data;
    
    if (data->generateProcess_)
    {
        // Stop the process before deleting it
        qDebug() << "LlamaCppService::clearData: Stopping generation process";
        data->generateProcess_->stop();
        delete data->generateProcess_;
        data->generateProcess_ = nullptr;
    }
    
    // Free context resources
    data->deinitialize();
    data->clear();
    data->model_ = nullptr;

    // Wait for GPU memory to be released
    // CUDA operations are asynchronous, we need to wait for the driver to release memory
    waitForGpuMemoryPurge();
    
    qDebug() << "LlamaCppService::clearData: Cleanup completed";
}

void LlamaCppService::clearModelInMemory(const QString& modelName)
{
    if (auto it = models_.find(modelName); it != models_.end())
    {
        LlamaModelData& model = it->second;

        if (currentModel_ == &model)
        {
            currentModel_ = nullptr;
            qDebug() << "LlamaCppService::clearModelInMemory ... currentModel_=nullptr";
        }

        if (!model.model_)
            return;

        qDebug() << "LlamaCppService::clearModelInMemory: " << modelName;
        
        llama_model_free(model.model_);

        waitForGpuMemoryPurge();
        bool check = checkGpuMemoryAvailable();

        model.model_ = nullptr;
    }
}

LlamaModelData* LlamaCppService::loadModel(const QString& modelName, int numGpuLayers, bool clearOtherModels)
{
    LlamaModelData modelData;

    qDebug() << "LlamaCppService::loadModel ... start loading model";

    const std::vector<LLMModel>& models = getAvailableModels();
    for (const LLMModel& model : models)
    {
        if (model.toString() == modelName)
        {
            qDebug() << "LlamaCppService::loadModel: model" << model.toString() << " file:" << model.filePath_;
            modelData.modelPath_ = model.filePath_;
            break;
        }
    }

    if (clearOtherModels)
    {
        qDebug() << "LlamaCppService::loadModel: clearOtherModels in memory";
        for (auto model : models_)
        {
            if (model.first != modelName && (embeddingModel_ && 
                model.first != embeddingModel_->modelName_))
            {
                clearModelInMemory(model.first);
                markAvailableModelsDirty();
            }

            if (currentModel_ && currentModel_ == &model.second)
            {
                currentModel_ = nullptr;
                qDebug() << "LlamaCppService::loadModel ... currentModel_=nullptr";
            }
        }
    }

    // initialize the model
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = numGpuLayers;

    qDebug() << "LlamaCppService::loadModel: Loading model with" << model_params.n_gpu_layers << "GPU layers";

    modelData.model_ = llama_model_load_from_file(qPrintable(modelData.modelPath_), model_params);
    if (!modelData.model_)
    {
        qDebug() << "llama-cpp error: unable to load model" << modelData.modelPath_;
        ErrorSystem::instance().log(ERRCODE_LLAMACPP_FAILED_LOAD_MODEL);
        return nullptr;
    }
    modelData.modelName_ = modelName;
    modelData.n_gpu_layers_ = numGpuLayers;
    modelData.use_gpu_ = numGpuLayers > 0;
    models_[modelName] = modelData;

    qDebug() << "LlamaCppService::loadModel ... end loading model";

    return &models_[modelName];
}

void LlamaCppService::setModelInternal(LlamaCppChatData* data, const QString& modelName)
{
    qDebug() << "LlamaCppService::setModelInternal ...";

    emit modelLoadingStarted(modelName);
    
    std::lock_guard<std::mutex> lock(modelMutex_);

    state_ = isLoading;

    if (data->model_ && modelName != data->model_->modelName_)
    {
        qDebug() << "LlamaCppService::setModelInternal ... change to model:" << data->model_->modelName_ << " -> " << modelName;
        clearData(data);
    }

    LlamaModelData* model = this->getModel(modelName);
    if (!model || !model->model_)                        
        model = this->loadModel(modelName, 99, onlyOneModelInMemory_);
    
    qDebug() << "LlamaCppService::setModelInternal ... data: " << data << " model: " << model;

    if (model && data->model_ != model)
        initializeData(data, model);

    if (model)
    {
        currentModel_ = model;
        qDebug() << "LlamaCppService::setModelInternal ... currentModel_: " << currentModel_;
    }

    state_ = isWaiting;

    emit modelLoadingFinished(modelName, true);

    qDebug() << "LlamaCppService::setModelInternal ... end!";
}

void LlamaCppService::deleteModel(const LLMModel& model)
{
    qDebug() << "LlamaCppService::deleteModel ... " << model.toString();
    
    clearModelInMemory(model.toString());
    markAvailableModelsDirty();
    QFile::remove(model.filePath_);
}

bool LlamaCppService::isReady() const
{
    return state_ == isWaiting;
}

void LlamaCppService::post(Chat* chat, const QString& content, bool streamed)
{
    qDebug() << "LlamaCppService::post ...";

    LlamaCppChatData* data = getData(chat);
    if (!data)
    {
        qDebug() << "LlamaCppService::post ... create data";
        data = createData(chat);
        if (!data)
        {
            qWarning() << "LlamaCppService::post ... no data";
            return;
        }
    }
    data->chat_ = chat;

    RAGService* ragService = ApplicationServices::get<RAGService>();

    QtConcurrent::run([this, data, chat]
        {
            if (state_ == isWaiting)
            {
                qDebug() << "LlamaCppService::post ... => setModelInternal";
                setModelInternal(data, chat->getCurrentModel());
            }
            else
            {   
                qDebug() << "LlamaCppService::post ... => setModelInternal ... already running ... skip and wait !";
                while (state_ == isLoading) QThread::msleep(100);
            }
        })        
        .then(
        [this, ragService, content]()
        {
            QString prompt;
            if (ragService && !ragService->isEmpty() && ragService->isEnabled())
            {
                qDebug() << "LlamaCppService::post ... use ragService";
                QString context = ragService->retrieveContext(this, content);
                if (!context.isEmpty())
                {
                    qDebug() << "LlamaCppService::post ... ragService: find context: " << context;
                    prompt = QString("Uses the following context to answer the user question:\n%1\n\nUser Question: %2").arg(context).arg(content);
                }
            }
            
            qDebug() << "LlamaCppService::post ... prompt:" << (!prompt.isEmpty() ? prompt : content);
            return !prompt.isEmpty() ? prompt : content;
        })
        .then(
        [this, data, chat, streamed](QString prompt) 
        {
            if (!data || !data->model_)
            {
                qWarning() << "LlamaCppService::post: no data or no model";
                chat->setProcessing(false);
                return;
            }
            
            // Check that context is properly initialized
            if (!data->ctx_)
            {
                qWarning() << "LlamaCppService::post: context not initialized - cannot generate";
                qWarning() << "Context could not be created, likely due to insufficient GPU memory";
                chat->setProcessing(false);
                return;
            }
            
            // Check that generation process exists
            if (!data->generateProcess_)
            {
                qWarning() << "LlamaCppService::post: generation process not initialized";
                chat->setProcessing(false);
                return;
            }
            
            chat->updateContent(prompt);
            data->generateProcess_->start(chat, prompt, streamed);
        });
}

QString LlamaCppService::formatMessages(const Chat* chat) const
{
    const LlamaCppChatData* data = getData(chat);
    if (!data || !data->ctx_)
        return {};

    const char* START_THINK = "<think>";
    const char* END_THINK = "</think>";

    QString entry, thought;
    std::vector<llama_chat_message> messages;
    const QList<ChatMessage>& history = chat->getHistory();
    for (const ChatMessage& message : history)
    {
        entry += message.role_ + ": " + message.content_;
        qDebug() << "LlamaCppApi::formatMessages: role:" << message.role_ << " content:" << message.content_;
        if (message.role_ == "thought")
            thought = START_THINK + message.content_ + END_THINK;
        else if (message.role_ == "user")
            messages.push_back({ "user", strdup(message.content_.toUtf8().constData()) });
        else if (message.role_ == "assistant" && !message.content_.isEmpty())
            messages.push_back({ "assistant", strdup((thought+message.content_).toUtf8().constData()) });
    }

    std::vector<char> formatted(llama_n_ctx(data->ctx_) * LLM_MAX_TOKEN_LEN);
    int new_len = llama_chat_apply_template(data->llamaCppChattemplate_, messages.data(),
                                            messages.size(), true, formatted.data(), formatted.size());
    
    for (llama_chat_message& msg : messages)
        free(const_cast<char*>(msg.content));

    return QString::fromUtf8(formatted.data(), new_len);
}

QString LlamaCppService::formatMessage(const Chat* chat, int historyIndex) const
{
    const LlamaCppChatData* data = getData(chat);
    const ChatMessage& message = chat->getHistory()[historyIndex];
    std::vector<llama_chat_message> messages;
    messages.push_back({ strdup(message.role_.toUtf8().constData()), strdup(message.content_.toUtf8().constData()) });
    std::vector<char> formatted(llama_n_ctx(data->ctx_) * LLM_MAX_TOKEN_LEN);
    int new_len = llama_chat_apply_template(
        data->llamaCppChattemplate_, messages.data(), messages.size(), true, formatted.data(), formatted.size());
    for (llama_chat_message& msg : messages)
    {
        free(const_cast<char*>(msg.role));
        free(const_cast<char*>(msg.content));
    }

    QString formattedStr = QString::fromUtf8(formatted.data(), new_len);
    qDebug() << "formatLastUserMessage: str:" << message.content_;
    qDebug() << "formatLastUserMessage: fmt:" << formattedStr;
    return formattedStr;
}

void LlamaCppService::stopStream(Chat* chat)
{
    LlamaCppChatData* data = getData(chat);
    if (data && data->generateProcess_)
        data->generateProcess_->stopProcess();
}

// GPU configuration methods
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
    info += "=== Available Backends ===\n";

    QStringList backends = getAvailableBackends();
    for (const QString& backend : backends)
    {
        info += "- " + backend + "\n";
    }

    info += "\n=== Available Devices ===\n";
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

const std::vector<LLMModel>& LlamaCppService::getAvailableModels()
{
    if (availableModelsDirty_)
    {
        std::vector<LLMModel> models;

        // LlamaCpp can use shared models from Ollama.
        if (llmservices_->hasSharedModels())
        {
            OllamaService::getOllamaModels(OllamaService::ollamaSystemDir, models);
            OllamaService::getOllamaModels(QDir::homePath() + "/", models);  
        }
        
        // with llamacppservice, the ollama cloud models are not available
        models.erase(std::remove_if(models.begin(), models.end(), 
            [](LLMModel& m){ return m.num_params_ == "cloud"; }), models.end());

        QStringList paths;
        paths += QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/models";
        if (!llmservices_->getCustomModelsPath().isEmpty())
            paths += llmservices_->getCustomModelsPath();

        for (const QString& path : paths)
        {
            qDebug() << "LlamaCppService::getAvailableModels: path:" << path;
            QDir appDataModelsDir(path);
            if (appDataModelsDir.exists())
            {
                QDirIterator it(path, QStringList() << "*.gguf", QDir::Files, QDirIterator::NoIteratorFlags);
                while (it.hasNext())
                {
                    it.next();
                    LLMModel model;
                    model.filePath_ = it.fileInfo().absoluteFilePath();
                    model.name_ = it.fileName().replace(".gguf", ""); // Use filename as model name
                    model.num_params_ = "";                           // Unknown without parsing GGUF header
                    models.push_back(model);
                }
            }
        }

        availableModels_ = models;
        availableModelsDirty_ = false;
        qDebug() << "LlamaCppService::getAvailableModels: updated: " << models.size() << " models found";
    }

    return availableModels_;
}

LlamaModelData* LlamaCppService::getModel(const QString& modelname)
{
    auto it = models_.find(modelname);
    return it != models_.end() ? &it->second : nullptr;
}

const LlamaModelData* LlamaCppService::getModel(const QString& modelname) const
{
    const auto it = models_.find(modelname);
    return it != models_.end() ? &it->second : nullptr;
}

LlamaCppChatData* LlamaCppService::getData(Chat* chat)
{
    auto it = datas_.find(chat);
    return it != datas_.end() ? &it->second : nullptr;
}

const LlamaCppChatData* LlamaCppService::getData(const Chat* chat) const
{
    const auto it = datas_.find(chat);
    return it != datas_.end() ? &it->second : nullptr;
}

std::vector<float> LlamaCppService::getEmbedding(const QString& text)
{   
    std::lock_guard<std::mutex> lock(modelMutex_);

    std::vector<float> embedding;

    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    bool decode = false;
    float* emb_ptr = nullptr;

    // Get a model
    // si un modele existe dejà, on le reprend pour traiter les embeddings
    if (currentModel_)
        model = currentModel_->model_;
    else if (embeddingModel_ && embeddingModel_->model_)
        model = embeddingModel_->model_;

    // load a model
    if (!model)
    {
        const std::vector<LLMModel>& models = getAvailableModels();       
        if (models.size())
        {
            // TODO : trouver un petit modele
            QString modelName = models.front().toString();
            embeddingModel_ = loadModel(modelName, 99, false);
            model = embeddingModel_->model_;
            qDebug() << "LlamaCppService::getEmbedding: loading model for embeddings" << modelName;
        }
        else
            qDebug() << "LlamaCppService::getEmbedding: no models";
    }       
    
    if (!model)
        return {};

    // Create Context for Embedding
    // We use a temporary context to avoid interfering with chat state
    llama_context_params params = llama_context_default_params();
    params.embeddings = true; // Enable embeddings
    // Use a reasonable context size for chunks (512 was chunk size, so 1024 or 2048 is safe)
    params.n_ctx = 2048;
    params.n_batch = 2048;
    ctx = LLamaInitializeContext(model, params, false);
    
    // Tokenize
    if (ctx)
    {
        std::vector<llama_token> tokens = LlamaTokenize(ctx, model, text);
        if (!tokens.empty())
        {
            llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
            decode = llama_decode(ctx, batch) == 0;
        }
    }
    // Extract Embedding
    if (decode)
        emb_ptr = llama_get_embeddings(ctx);

    // Normalize (Cosine Similarity requires normalized vectors or division by norm)
    // We normalize here for storage efficiency and search speed
    if (emb_ptr)
    {
        int n_embd = llama_model_n_embd(model);
        embedding = std::vector<float>(emb_ptr, emb_ptr + n_embd);

        float norm = 0.0f;
        for (float f : embedding)
            norm += f * f;
        norm = std::sqrt(norm);

        if (norm > 1e-6) // Avoid div by zero
        {
            for (float& f : embedding)
                f /= norm;
        }     
    }

    if (ctx)
        llama_free(ctx);

    if (!emb_ptr)
        qWarning() << "LlamaCppService::getEmbedding: error !";

    qDebug() << "LlamaCppService::getEmbedding ... end";
    return embedding;       
}
