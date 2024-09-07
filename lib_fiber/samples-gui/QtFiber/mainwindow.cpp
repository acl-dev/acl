#include "stdafx.h"
#include <QMessageBox>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "childwindows.h"
#include "fiber_server.h"
#include "fiber_client.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
    , process_(new QProcess(this))
{
    ui_->setupUi(this);

    button_ = new QPushButton("Open Dialog", this);
    button_->setGeometry(QRect(QPoint(100, 100), QSize(300, 50)));
    connect(button_, &QPushButton::clicked, this, &MainWindow::onButtonClicked);

    start_server_ = new QPushButton("Start fiber server", this);
    start_server_->setGeometry(QRect(QPoint(100, 200), QSize(300, 50)));
    connect(start_server_, &QPushButton::clicked, this, &MainWindow::onStartServer);

    start_client_ = new QPushButton("Start fiber client", this);
    start_client_->setGeometry(QRect(QPoint(100, 300), QSize(300, 50)));
    connect(start_client_, &QPushButton::clicked, this, &MainWindow::onStartClient);

    stop_fiber_ = new QPushButton("Stop fiber schedule", this);
    stop_fiber_->setGeometry(QRect(QPoint(100, 400), QSize(300, 50)));
    connect(stop_fiber_, &QPushButton::clicked, this, &MainWindow::onStopSchedule);

    open_child_ = new QPushButton("Open Child Window", this);
    open_child_->setGeometry(QRect(QPoint(100, 500), QSize(300, 50)));
    connect(open_child_, &QPushButton::clicked, this, &MainWindow::onOpenChildWindow);
}

MainWindow::~MainWindow()
{
    delete ui_;
    delete server_;
}

void MainWindow::onAboutToQuit()
{
}

void MainWindow::onButtonClicked()
{
    QMessageBox::information(this, "Dialog Title", "This is a message in the dialog.");
}

void MainWindow::onStartServer()
{
    if (server_ != nullptr) {
        qDebug() << "Fiber server is running!";
        return;
    }

    server_ = new fiber_server("127.0.0.1", 9001, this);
    qDebug() << "Start one fiber";
    server_->start();
    qDebug() << "Fiber started\r\n";
}

void MainWindow::onStartClient()
{
    acl::fiber *fb = new fiber_client("127.0.0.1", 9001, 10000, 0);
    qDebug() << "Start fiber client";
    fb->start();
    qDebug() << "Fiber client started!";
}

void MainWindow::onStopSchedule()
{
    if (server_) {
        server_->stop();
        delete server_;
        server_ = nullptr;
    }

    acl::fiber::schedule_stop();
    qDebug() << "Fiber schedule stopped!";
}

void MainWindow::onOpenChildWindow()
{
    if (child_window_ == nullptr) {
        child_window_ = new ChildWindows(this);
    }

    qDebug() << "Opening second window";

    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        int x = (screenGeometry.width() - child_window_->width()) / 2;
        int y = (screenGeometry.height() - child_window_->height()) / 2;
        child_window_->move(x, y);
    }

    child_window_->show();
    child_window_->raise();
    child_window_->activateWindow();
    qDebug() << "Second window visibility:" << child_window_->isVisible();
    qDebug() << "Second window isMinimized:" << child_window_->isMinimized();
    qDebug() << "Second window isActiveWindow:" << child_window_->isActiveWindow();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        if (event->key() == Qt::Key_N) {

        }
    }
}
