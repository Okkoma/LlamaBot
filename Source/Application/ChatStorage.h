#pragma once

#include <QObject>
#include <QString>

#include "LLMServices.h"

/**
 * @brief Interface de stockage des chats (local, distant, synchronisé, etc.)
 *
 * Contrat minimal: sérialisation/désérialisation via le format JSON existant
 * (Chat::toJson/fromJson) pour minimiser l'impact sur le reste de l'application.
 */
class ChatStorage : public QObject
{
    Q_OBJECT
public:
    explicit ChatStorage(LLMServices* llmservices) : QObject(llmservices), llmServices_(llmservices) {}
    virtual ~ChatStorage() = default;
    
    virtual bool load(QList<Chat*>& chats) = 0;
    virtual bool save(const QList<Chat*>& chats) = 0;

protected:
    LLMServices* llmServices_;
};

