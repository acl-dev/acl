#include "stdafx.h"
#include <QMessageBox>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "inputdialog.h"
#include "fiber_server.h"
#include "fiber_client.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
    , process_(new QProcess(this))
{
    ui_->setupUi(this);
    stamp_ = new struct timeval;
    gettimeofday(stamp_, nullptr);

    connect(ui_->clear, &QPushButton::clicked, this, &MainWindow::onClear);
    connect(ui_->startSchedule, &QPushButton::clicked, this, &MainWindow::onStartSchedule);
    connect(ui_->stopSchedule, &QPushButton::clicked, this, &MainWindow::onStopSchedule);
    connect(ui_->startServer, &QPushButton::clicked, this, &MainWindow::onStartServer);
    connect(ui_->stopServer, &QPushButton::clicked, this, &MainWindow::onStopServer);
    connect(ui_->startClient, &QPushButton::clicked, this, &MainWindow::onStartClient);
    connect(ui_->urlGet, &QPushButton::clicked, this, &MainWindow::onUrlGet);

    ui_->startSchedule->setEnabled(false);
    ui_->stopServer->setEnabled(false);
    ui_->startClient->setEnabled(false);
}

MainWindow::~MainWindow()
{
    delete ui_;
    delete stamp_;
    delete server_;
    qDebug() << "The main windows was destroied!";
}

void MainWindow::closeEvent(QCloseEvent *event)
{
#if 1
    acl::fiber::schedule_stop(); // 停止协程调度器
    event->accept();  // 接受关闭事件
    return;
#else
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Confirm Close",
        tr("Are you sure you want to exit?\n"),
        QMessageBox::No | QMessageBox::Yes,
        QMessageBox::No);

    if (resBtn == QMessageBox::Yes) {
        acl::fiber::schedule_stop();
        event->accept();  // 接受关闭事件
    } else {
        event->ignore();  // 忽略关闭事件
    }
#endif
}

void MainWindow::onClear()
{
    ui_->display->clear();
}

void MainWindow::setProgress(int n)
{
    if (n == 100) {
        ui_->progress->setValue(100);
    } else {
        struct timeval now;
        gettimeofday(&now, nullptr);
        double tc = acl::stamp_sub(now, *stamp_);
        if (tc >= 200) {
            ui_->progress->setValue(n % 100);
            gettimeofday(stamp_, nullptr);
        }
    }
}

void MainWindow::onAboutToQuit()
{
    qDebug() << "onAboutToQuit called!";
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

    ui_->stopServer->setEnabled(true);
    ui_->startClient->setEnabled(true);

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
        ui_->stopServer->setEnabled(false);
        ui_->startClient->setEnabled(false);
    }
}

void MainWindow::onStartClient()
{
    if (!acl::fiber::scheduled()) {
        qDebug() << "gui fiber not scheduled yet!";
        return;
    }

    ui_->progress->setValue(0);
    acl::fiber *fb = new fiber_client(this, "127.0.0.1", 9001, 10000, 1);
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
        ui_->display->setText(buf.c_str());
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

    ui_->stopSchedule->setEnabled(true);
    ui_->urlGet->setEnabled(true);

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

    ui_->urlGet->setEnabled(false);
    ui_->stopSchedule->setEnabled(false);
    ui_->startSchedule->setEnabled(true);

    acl::fiber::schedule_stop();
    qDebug() << "Fiber schedule stopped!";
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
    //input_display_->setText("输入内容: " + text);
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        if (event->key() == Qt::Key_N) {

        }
    }
}
