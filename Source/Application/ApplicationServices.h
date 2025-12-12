#pragma once

#include <memory>

#include <QObject>
#include <QMap>

#include "LLMService.h"

class ApplicationServices : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationServices(QObject* parent);
    ~ApplicationServices();
    
    template<typename T> static T* get() { return reinterpret_cast<T*>(services_[T::staticMetaObject.className()].get()); } 

private:
    static std::unordered_map<const char*, std::unique_ptr<QObject> > services_;
};