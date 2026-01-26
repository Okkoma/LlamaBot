#pragma once

#include <QObject>

class ThemeManager : public QObject
{
    Q_OBJECT
    
public:
    ThemeManager(QObject* parent = nullptr) : QObject(parent) { }
    ~ThemeManager() override { }

    const QString& currentFont() const { static QString sThemeManagerCurrentFont_ = "default"; return sThemeManagerCurrentFont_; }    
    int currentFontSize() const { return 11; }
};

class ApplicationServices : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationServices(QObject* parent) : QObject(parent) { initialize(); }
    ~ApplicationServices() override { services_.clear(); }
    
    template<typename T> static T* get() { return reinterpret_cast<T*>(services_[T::staticMetaObject.className()].get()); }

    void initialize();

    static std::unordered_map<const char*, std::unique_ptr<QObject> > services_;
};
