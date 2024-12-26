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
struct timeval;

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

    void setProgress(int n);

protected:
    void keyPressEvent(QKeyEvent *event);

    void onButtonClicked();
    void onStartServer();
    void onStopServer();
    void onStartClient();
    void onStartSchedule();
    void onStopSchedule();
    void onHttpOptions();
    void onUrlGet();

    void onClear();

    void onDialogAccepted(const QString &text);
    void closeEvent(QCloseEvent *event) override;

public:
    void onAboutToQuit();

private:
    void onDownloadFinish(bool ok, const acl::http_request& req);

private:
    Ui::MainWindow *ui_;

    fiber_server   *server_ = nullptr;
    struct timeval *stamp_;

    std::string url_ = "http://www.baidu.com/";
    std::string host_;
    std::map<std::string, std::string> headers_;
};
#endif // MAINWINDOW_H
