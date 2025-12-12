
#include <QDebug>

#include "ApplicationServices.h"


std::unordered_map<const char*, std::unique_ptr<QObject> > ApplicationServices::services_;

ApplicationServices::ApplicationServices(QObject* parent) :
    QObject(parent)
{
    // add service
    services_.emplace(std::make_pair(LLMService::staticMetaObject.className(), std::unique_ptr<LLMService>(new LLMService(this))));
    qDebug() << "ApplicationServices: add" << get<LLMService>();
}

ApplicationServices::~ApplicationServices()
{
    services_.clear();
    qDebug() << "~ApplicationServices";
}