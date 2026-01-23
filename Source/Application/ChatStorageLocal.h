#pragma once

#include <QSqlDatabase>

#include "ChatStorage.h"

/**
 * @brief Stockage local des chats dans SQLite (via QtSql / driver QSQLITE)
 */
class ChatStorageLocal final : public ChatStorage
{
    Q_OBJECT
public:
    explicit ChatStorageLocal(LLMServices* llmservices);
    ~ChatStorageLocal() override;

    bool load(QList<Chat*>& chats) override;
    bool save(const QList<Chat*>& chats) override;

private:
    QString dbPath() const;
    QString jsonfilePath() const;
    bool isAvailable() const;
    bool openDatabase();

    bool loadBinaryDb(QList<Chat*>& chats);
    bool saveBinaryDb(const QList<Chat*>& chats);

    std::optional<QJsonArray> loadJsonDb();
    bool saveJsonDb(const QJsonArray& chats);
    
    std::optional<QJsonArray> loadJsonFile();
    bool saveJsonFile(const QJsonArray& chats);

    QString connectionName_;
    QSqlDatabase db_;
};

