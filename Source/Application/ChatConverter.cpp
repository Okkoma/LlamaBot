#include "ChatImpl.h"

bool convertJsonToChatList(const QJsonArray& jsonArray, QList<Chat*>& chats, LLMServices* llmservices)
{
    for (const auto& val : jsonArray)
    {
        if (!val.isObject())
            continue;

        // Appel à la factory Chat
        Chat* chat = ChatImpl::Create(llmservices, "", "", true);
        chat->fromJson(val.toObject());
        chats.append(chat);
    }     
    return true;
}

QJsonArray convertChatListToJson(const QList<Chat*>& chats)
{
    QJsonArray array;
    for (Chat* chat : chats)    
        array.append(chat->toJson());    
    return array;
}