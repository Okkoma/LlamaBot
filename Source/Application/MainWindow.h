#pragma once

#include <QMainWindow>

class QWidget;
class QAction;
class ChatBotPanel;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MainWindow() override;
        
    void setStyle(const QString& styleName);

signals:
    void styleChanged(const QString& style);

private slots:
    void Action_quitApplication();
    void Action_setMaterialDarkStyle();
    void Action_setMaterialLightStyle();

private:
    void initialize();

    // Menu Actions
    QAction* materialDarkStyleAction_;
    QAction* materialLightStyleAction_;

    // ChatBot Panel
    ChatBotPanel* chatBotWidget_;
};
