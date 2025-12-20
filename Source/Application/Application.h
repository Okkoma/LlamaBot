#pragma once

#include <QApplication>
#include <QFont>

#include "ApplicationServices.h"

class QQmlApplicationEngine;
class ChatController;
class OllamaModelStoreDialog;
class ThemeManager;

class Application : public QApplication
{
    Q_OBJECT

public:
    explicit Application(int& argc, char** argv);
    ~Application() override;


private:
    QQmlApplicationEngine* qmlEngine_;
    ChatController* chatController_;
    OllamaModelStoreDialog* modelStoreDialog_;
    ApplicationServices services_;
    ThemeManager* themeManager_;
};
