#pragma once

#include <QObject>
#include <QQmlEngine>

class ErrorMessenger : public QObject
{
    Q_OBJECT

    QML_ELEMENT
    QML_SINGLETON
    QML_UNCREATABLE("ErrorMessenger is a singleton provided by the application")

public:
    ErrorMessenger(QObject* parent = nullptr);

    Q_INVOKABLE void log(int code, const QStringList& params = QStringList());

    Q_INVOKABLE QVariantList get(int count=-1) const;
    
signals:
    void errorPopped(QString msg);
};

