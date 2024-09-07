#include "stdafx.h"
#include <QMessageBox>
#include "mainwindow.h"
#include "fiber_echo.h"
#include "fiber_server.h"

fiber_server::fiber_server(const char *ip, int port, MainWindow *parent)
: ip_(ip), port_(port), parent_(parent)
{
    box_ = new acl::fiber_tbox<bool>;
}

fiber_server::~fiber_server() {
    delete box_;
}

void fiber_server::stop() {
    this->kill();
    box_->pop();
}

void fiber_server::run() {
    QMessageBox::information(parent_, "fiber-server", "This dialog is in fiber server!");
    server_start();
}

void fiber_server::server_start() {
    const char *ip = "127.0.0.1";
    int port = 9001;
    SOCKET sock = socket_listen(ip_.c_str(), port_);
    if (sock == INVALID_SOCKET) {
        qDebug() << "Open " << ip << ":" << port << " error: " << acl::last_serror();
        return;
    }
    qDebug() << "Open ok, addr=" << ip << ":" << port;

    while (true) {
        SOCKET conn = socket_accept(sock);
        if (conn == INVALID_SOCKET) {
            qDebug() << "Accept failed: " << (acl::fiber::killed() ? "killed" : acl::last_serror());
            break;
        }

        acl::fiber* fb = new fiber_echo(conn);
        fb->start();
    }

    socket_close(sock);
    box_->push(nullptr);
}
