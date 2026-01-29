#pragma once

#include <QQmlEngine>
#include <QClipboard>
#include <QGuiApplication>
#include <QObject>

class Clipboard : public QObject
{
    Q_OBJECT

    QML_UNCREATABLE("Clipboard is a singleton provided by the application")

public:
    explicit Clipboard(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void setText(const QString& text);
    Q_INVOKABLE QString getText() const;

    Q_INVOKABLE bool hasUrls() const;
    Q_INVOKABLE QStringList getUrls() const;

    Q_INVOKABLE bool hasImage() const;
    Q_INVOKABLE QString getImageAsBase64() const;
    Q_INVOKABLE QString fileToBase64(const QString& filePath) const;
};
