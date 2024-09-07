#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QKeyEvent>
#include <QPushButton>
#include <QProcess.h>

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
    void onStartClient();
    void onStopSchedule();
    void onOpenChildWindow();

public:
    void onAboutToQuit();

private:
    Ui::MainWindow *ui_;
    QPushButton    *button_;
    QPushButton    *start_server_;
    QPushButton    *start_client_;
    QPushButton    *stop_fiber_;
    QPushButton    *open_child_;

    fiber_server   *server_ = nullptr;
    QProcess       *process_;

    ChildWindows   *child_window_ = nullptr;
};
#endif // MAINWINDOW_H
