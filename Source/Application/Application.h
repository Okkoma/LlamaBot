#pragma once

#include <memory>

#include <QApplication>
#include <QFont>

#include "ApplicationServices.h"

class MainWindow;
class QQmlApplicationEngine;
class ChatController;
class OllamaModelStoreDialog;

class Application : public QApplication
{
    Q_OBJECT
    Q_PROPERTY(QString currentTheme READ currentTheme NOTIFY themeChanged)

public:
    explicit Application(int& argc, char** argv);
    ~Application() override;

    void setFont(const QFont& font);

    void saveSettings();
    void loadSettings();
    void setStyleName(const QString& styleName);
    QString styleName() const;

    // Theme management for QML
    Q_INVOKABLE void setTheme(const QString& theme);
    QString currentTheme() const;

signals:
    void themeChanged(const QString& theme);

private slots:
    void ApplyStyle(const QString& style);

private:
    bool eventFilter(QObject* obj, QEvent* event) override;

    MainWindow* window_;
    QQmlApplicationEngine* qmlEngine_;
    ChatController* chatController_;
    OllamaModelStoreDialog* modelStoreDialog_;
    ApplicationServices services_;

    QFont currentFont_;
    QString currentStyleName_ = "Fusion";
    QString currentTheme_ = "Dark";
};
