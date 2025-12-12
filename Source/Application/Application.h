#pragma once

#include <memory>

#include <QApplication>
#include <QFont>

#include "ApplicationServices.h"


class MainWindow;

class Application : public QApplication
{
    Q_OBJECT
    
public:
    explicit Application(int& argc, char **argv);
    ~Application() override;

    void setFont(const QFont& font);

    void saveSettings();
    void loadSettings();
    void setStyleName(const QString& styleName);
    QString styleName() const;

private slots:
    void ApplyStyle(const QString& style);

private:
    bool eventFilter(QObject* obj, QEvent* event) override;

    MainWindow* window_;
    ApplicationServices services_;

    QFont currentFont_;
    QString currentStyleName_ = "Fusion";
};
