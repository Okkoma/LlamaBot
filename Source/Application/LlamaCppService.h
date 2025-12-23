#pragma once

#include <QFuture>

#include "llama-cpp.h"

#include "LLMServiceDefs.h"

struct LlamaCppChatData;

struct LlamaCppProcess
{
    LlamaCppProcess(int type, LlamaCppChatData* data, LLMService* service) : type_(type), data_(data), service_(service)
    {
    }
    virtual ~LlamaCppProcess() {}

    virtual void start(Chat* chat, const QString& content, bool streamed) = 0;
    virtual void stop() = 0;
    virtual void stopProcess() = 0;
    virtual void generate() {}

    int type_;
    LlamaCppChatData* data_;
    LLMService* service_;
};

struct LlamaModelData
{
    QString modelName_;
    QString modelPath_;
    int n_gpu_layers_ = 99; // Nombre de couches à charger sur GPU (99 = toutes)
    bool use_gpu_ = true;   // Activer/désactiver GPU
    llama_model* model_ = nullptr;
};

struct LlamaCppChatData
{
    LlamaCppChatData() = default;
    ~LlamaCppChatData();

    void initialize(LlamaModelData* model = nullptr);
    void deinitialize();

    Chat* chat_ = nullptr;

    LlamaModelData* model_ = nullptr;
    int n_ctx_ = 2048; // Taille du contexte
    llama_context* ctx_ = nullptr;
    llama_sampler* smpl_ = nullptr;
    const char* llamaCppChattemplate_ = nullptr;

    llama_batch batch_;
    llama_token tokenId_;

    std::vector<llama_token> tokens_;
    std::vector<llama_chat_message> llamaCppChatMessages_;

    QString response_;

    QString pendingModelName_;
    QFuture<void> loadingFuture_;

    LlamaCppProcess* generateProcess_ = nullptr;
};

class LlamaCppWorker : public QObject
{
    Q_OBJECT

public:
    explicit LlamaCppWorker(LlamaCppProcess* process);
    ~LlamaCppWorker();

public slots:
    void processRequest();
    void stopProcessing();

signals:
    void tokenGenerated(const QString& token);
    void generationFinished();
    void errorOccurred(const QString& error);

private:
    LlamaCppProcess* process_;
    QThread* thread_;
};

class LlamaCppService : public LLMAPIEntry
{
    Q_OBJECT

public:
    LlamaCppService(LLMService* service, const QString& name);
    LlamaCppService(const QVariantMap& params);
    ~LlamaCppService() override;

    void setModel(Chat* chat, QString model = "") override;

    bool isReady() const override;

    void post(Chat* chat, const QString& content, bool streamed = true) override;
    QString formatMessage(Chat* chat, const QString& role, const QString& content) override;
    void stopStream(Chat* chat) override;

    QJsonObject toJson() const override;

    LlamaCppChatData* createData(Chat* chat);
    void initializeData(LlamaCppChatData* data, LlamaModelData* model = nullptr);
    void clearData(LlamaCppChatData* data);
    void clearModelInMemory(const QString& modelName);

    // Activer/désactiver la version threadée
    void setUseThreadedVersion(bool useThreaded) { useThreadedVersion_ = useThreaded; }
    bool isUsingThreadedVersion() const { return useThreadedVersion_; }

    // Configuration GPU
    void setDefaultGpuLayers(int n_gpu_layers);
    void setDefaultContextSize(int n_ctx);
    void setDefaultUseGpu(bool use_gpu);

    int getGpuLayers(Chat* chat = nullptr) const;
    int getContextSize(Chat* chat = nullptr) const;
    bool isUsingGpu(Chat* chat = nullptr) const;

    LlamaModelData* getModel(const QString& modelname);
    const LlamaModelData* getModel(const QString& modelname) const;
    std::vector<LLMModel> getAvailableModels() const override;
    LlamaCppChatData* getData(Chat* chat);
    const LlamaCppChatData* getData(Chat* chat) const;

    std::vector<float> getEmbedding(const QString& text) override;

    // Informations sur les backends disponibles
    static QStringList getAvailableBackends();
    static QString getBackendInfo();
    static LlamaCppService* createDefault(LLMService* service, const QString& name);

    QHash<QString, LlamaModelData> models_;
    QHash<Chat*, LlamaCppChatData> datas_;

    int defaultGpuLayers_ = 99;
    int defaultContextSize_ = 2048;
    bool defaultUseGpu_ = true;
    bool useThreadedVersion_ = false;
    bool onlyOneModelInMemory_ = true;

private:
    LlamaModelData* loadModel(const QString& model, int numGpuLayers, bool clearOtherModels);
    void setModelInternal(LlamaCppChatData* data, const QString& modelName);

    LlamaModelData* lastModelAddedInMemory_ = nullptr;
    LlamaModelData* embeddingModel_ = nullptr;
};
