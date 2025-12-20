#pragma once

#include <QClipboard>
#include <QGuiApplication>
#include <QObject>

class Clipboard : public QObject
{
    Q_OBJECT
public:
    explicit Clipboard(QObject* parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void setText(const QString& text)
    {
        QClipboard* clipboard = QGuiApplication::clipboard();
        clipboard->setText(text);
    }
};
