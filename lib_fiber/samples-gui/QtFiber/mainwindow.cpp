#include "stdafx.h"
#include <QMessageBox>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "childwindows.h"
#include "inputdialog.h"
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
    start_server_->setGeometry(QRect(QPoint(100, 150), QSize(300, 50)));
    connect(start_server_, &QPushButton::clicked, this, &MainWindow::onStartServer);

    stop_server_ = new QPushButton("Stop fiber server", this);
    stop_server_->setGeometry(QRect(QPoint(100, 200), QSize(300, 50)));
    connect(stop_server_, &QPushButton::clicked, this, &MainWindow::onStopServer);

    start_client_ = new QPushButton("Start fiber client", this);
    start_client_->setGeometry(QRect(QPoint(100, 250), QSize(300, 50)));
    connect(start_client_, &QPushButton::clicked, this, &MainWindow::onStartClient);

    url_get_ = new QPushButton("Http download", this);
    url_get_->setGeometry(QRect(QPoint(100, 300), QSize(300, 50)));
    connect(url_get_, &QPushButton::clicked, this, &MainWindow::onUrlGet);

    start_schedule_ = new QPushButton("Start fiber schedule", this);
    start_schedule_->setGeometry(QRect(QPoint(100, 350), QSize(300, 50)));
    connect(start_schedule_, &QPushButton::clicked, this, &MainWindow::onStartSchedule);

    stop_schedule_ = new QPushButton("Stop fiber schedule", this);
    stop_schedule_->setGeometry(QRect(QPoint(100, 400), QSize(300, 50)));
    connect(stop_schedule_, &QPushButton::clicked, this, &MainWindow::onStopSchedule);

    open_child_ = new QPushButton("Open Child Window", this);
    open_child_->setGeometry(QRect(QPoint(100, 450), QSize(300, 50)));
    connect(open_child_, &QPushButton::clicked, this, &MainWindow::onOpenChildWindow);

    input_button_= new QPushButton("Open dialog", this);
    input_button_->setGeometry(100, 500, 300,50);
    connect(input_button_, &QPushButton::clicked, this, &MainWindow::onInputClicked);

    input_display_ = new QLabel("输入内容: ", this);
    input_display_->setGeometry(100, 550, 200, 50);
}

MainWindow::~MainWindow()
{
    delete ui_;
    delete server_;
    qDebug() << "The main windows was destroied!";
}

void MainWindow::onAboutToQuit()
{
    qDebug() << "onAboutToQuit called!";
//    acl::fiber::schedule_stop();
}

void MainWindow::onButtonClicked()
{
    QMessageBox::information(this, "Dialog Title", "This is a message in the dialog.");
}

void MainWindow::onStartServer()
{
    if (!acl::fiber::scheduled()) {
        qDebug() << "gui fiber not scheduled yet!";
        return;
    }

    if (server_ != nullptr) {
        qDebug() << "Fiber server is running!";
        return;
    }

    server_ = new fiber_server("127.0.0.1", 9001, this);
    qDebug() << "Start fiber server";
    server_->start();
    qDebug() << "Fiber server started";
}

void MainWindow::onStopServer()
{
    if (server_) {
        server_->stop();
        delete server_;
        server_ = nullptr;
    }
}

void MainWindow::onStartClient()
{
    if (!acl::fiber::scheduled()) {
        qDebug() << "gui fiber not scheduled yet!";
        return;
    }

    acl::fiber *fb = new fiber_client("127.0.0.1", 9001, 10000, 0);
    qDebug() << "Start fiber client";
    fb->start();
    qDebug() << "Fiber client started!";
}

void MainWindow::onUrlGet()
{
    if (!acl::fiber::scheduled()) {
        qDebug() << "gui fiber not scheudled yet!";
        return;
    }

    go[this] {
        const char *addr = "www.baidu.com:80";
        const char *host = "www.baidu.com";
        acl::http_request req(addr);
        req.request_header()
                .set_url("/")
                .set_host(host);
        if (!req.request(nullptr, 0)) {
            qDebug() << "Send http request to " << addr << " error: " << acl::last_serror();
            return;
        }

        acl::string buf;
        if (req.get_body(buf)) {
            this->onDownloadFinish(true, req);
        } else {
            this->onDownloadFinish(false, req);
        }
    };
}

void MainWindow::onDownloadFinish(bool ok, const acl::http_request& req)
{
    if (ok) {
        acl::http_client *client = req.get_client();
        acl::string buf;
        client->sprint_header(buf);
        qDebug() << "Got response body ok!";
        qDebug() << buf.c_str();
    } else {
        qDebug() << "Got response body error!";
    }
}

void MainWindow::onStartSchedule()
{
    if (acl::fiber::scheduled()) {
        qDebug() << "Fiber has been scheduled before!";
        return;
    }

    qDebug() << "Begin schedule_gui!";
    acl::fiber::schedule_gui();
    qDebug() << "schedule_gui end!";
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

void MainWindow::onInputClicked()
{
    InputDialog dialog(this);
    QRect mainWindowGeometry = this->frameGeometry();
    QPoint mainWindowPos = this->pos();
    int x = mainWindowPos.x() + (mainWindowGeometry.width() - dialog.width()) / 2;
    int y = mainWindowPos.y() + (mainWindowGeometry.height() - dialog.height()) / 2;
    dialog.move(x, y);
    connect(&dialog, &InputDialog::dialogAccepted, this, &MainWindow::onDialogAccepted);
    dialog.exec();
}

void MainWindow::onDialogAccepted(const QString &text)
{
    input_display_->setText("输入内容: " + text);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        if (event->key() == Qt::Key_N) {

        }
    }
}
