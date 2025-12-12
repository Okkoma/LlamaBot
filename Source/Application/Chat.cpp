#include <QTextEdit>
#include <QTextBrowser>
#include <QTextCursor>
#include <QScrollBar>

#include "Chat.h"
#include "LLMService.h"

//const char* rawHumanPrompt = "human:";
//const char* rawAiPrompt = "ai:";

const char* rawHumanPrompt = "";
const char* rawAiPrompt = "";
const char* rawDefaultInitialPrompt = "";


Chat::Chat(LLMService* service, const QString& name, const QString& initialPrompt, bool streamed) : 
    service_(service),
    streamed_(streamed),
    lastBotIndex_(-1),    
    name_(name),
    currentApi_("Ollama"),
    currentModel_("llama3.1:8b"),
    userPrompt_("🧑 >"),
    aiPrompt_("🤖 >")
{ 
    initialize(); 

    rawMessages_ = !initialPrompt.isEmpty() ? initialPrompt : rawDefaultInitialPrompt;
}

void Chat::initialize()
{
    lastScrollValue_ = 0;

    jsonObject_["model"] = currentModel_;
    jsonObject_["stream"] = streamed_;

    history_.clear();
}

void Chat::setApi(const QString& api)
{
    currentApi_ = api;
}

void Chat::setModel(const QString& model)
{
    LLMAPIEntry* api = service_->get(currentApi_);
    if (api)
        api->setModel(this, model);

    currentModel_ = model;
    jsonObject_["model"] = model;
}

void Chat::updateContent(const QString& content)
{
    // Ajoute le message utilisateur
    addContent("user", content);
    // Ajoute un bloc IA vide et mémorise son index
    addContent("assistant", "");
    // Affiche tout
    chatView_->setMarkdown(messages_.join("\n\n"));

    askText_->clear();

    updateObject();
}

void Chat::addContent(const QString& role, const QString& content)
{
    qDebug() << "Chat::addContent: role:" << role << "content:" << content;

    bool isUserContent = (role == "user");
    if (isUserContent)
        finalizeStream();

    // Add to history
    if (!content.isEmpty())
        history_.append({role, content});

    if (!content.isEmpty())
    {
        messages_.append(QString("%1 %2\n").arg(isUserContent ? userPrompt_ : aiPrompt_, content));

        LLMAPIEntry* api = service_->get(currentApi_);
        rawMessages_ += api ? api->formatMessage(this, role, content) : content + "\n";
    }
    else
    {
        messages_.append("");
        lastBotIndex_ = messages_.size() - 1;
    }    
}

void Chat::finalizeStream()
{
    // finalize to add the last streamed response
    if (!currentAIStream_.isEmpty())
    {
        qDebug() << "Chat::finalizeStream: ";

        LLMAPIEntry* api = service_->get(currentApi_);
        rawMessages_ += api ? api->formatMessage(this, "assistant", currentAIStream_) : currentAIStream_ + "\n";
        
        // Add valid response to history
        history_.append({"assistant", currentAIStream_});
        
        currentAIStream_.clear();
    }
}

void Chat::updateCurrentAIStream(const QString& text)
{
    int scrollValue = lastScrollValue_;

    bool atbottom = scrollValue >= chatView_->verticalScrollBar()->maximum() - 4;

    if (!text.isEmpty())
    {
        currentAIStream_ += text;

        // some sanitization
        if (currentAIStream_.startsWith("|"))
            currentAIStream_.removeFirst();

        if (currentAIStream_.startsWith(">"))
        {
            currentAIStream_.removeFirst();
            if (currentAIStream_.startsWith("\n"))
                currentAIStream_.removeFirst();
        }

        messages_[lastBotIndex_] = QString("%1 %2\n").arg(aiPrompt_, currentAIStream_);
    }

    chatView_->setMarkdown(messages_.join("\n\n"));
    
    // update scroll bar
    if (atbottom)    
    {
        chatView_->moveCursor(QTextCursor::End);
        chatView_->ensureCursorVisible();
    }
    else
        chatView_->verticalScrollBar()->setValue(scrollValue);
}

void Chat::updateObject()
{
    jsonObject_["prompt"] = rawMessages_;
}