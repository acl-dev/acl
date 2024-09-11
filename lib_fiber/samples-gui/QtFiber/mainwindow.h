#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QPushButton>
#include <QProcess.h>
#include <QLabel>

namespace acl {
    class http_request;
}

class fiber_server;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ChildWindows;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event);

    void onButtonClicked();
    void onStartServer();
    void onStopServer();
    void onStartClient();
    void onStartSchedule();
    void onStopSchedule();
    void onOpenChildWindow();
    void onInputClicked();
    void onUrlGet();

    void onDialogAccepted(const QString &text);

public:
    void onAboutToQuit();

private:
    void onDownloadFinish(bool ok, const acl::http_request& req);

private:
    Ui::MainWindow *ui_;
    QPushButton    *button_;
    QPushButton    *start_server_;
    QPushButton    *stop_server_;
    QPushButton    *start_client_;
    QPushButton    *start_schedule_;
    QPushButton    *stop_schedule_;
    QPushButton    *open_child_;
    QPushButton    *url_get_;
    std::string     url_;
    QPushButton    *input_button_;
    QLabel         *input_display_;

    fiber_server   *server_ = nullptr;
    QProcess       *process_;

    ChildWindows   *child_window_ = nullptr;
};
#endif // MAINWINDOW_H
