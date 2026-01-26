#pragma once

#include <memory>

#include <QObject>
#include <QMap>

class ApplicationServices : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationServices(QObject* parent);
    ~ApplicationServices();
    
    template<typename T> static T* get()
    { 
        return reinterpret_cast<T*>(services_[T::staticMetaObject.className()].get());
    } 

    template<typename T> static void add(QObject* parent)
    {
        services_.emplace(std::make_pair(T::staticMetaObject.className(), std::unique_ptr<T>(new T(parent))));
    }

    void initialize();

private:
    static std::unordered_map<const char*, std::unique_ptr<QObject> > services_;
};