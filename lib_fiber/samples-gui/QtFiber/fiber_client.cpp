#include "stdafx.h"
#include "patch.h"
#include "mainwindow.h"
#include "fiber_client.h"

fiber_client::fiber_client(MainWindow *parent, const char *ip, int port, size_t max, int delay)
: parent_(parent), ip_(ip), port_(port), max_(max), delay_(delay)
{
}

void fiber_client::run() {
    SOCKET conn = socket_connect(ip_.c_str(), port_);
    if (conn == INVALID_SOCKET) {
        qDebug() << "connect error: " << acl::last_serror() << ", addr: " << ip_.c_str() << ":" << port_;
        return;
    }

    qDebug() << "Fiber-" << acl::fiber::self() << " connect ok, addr: " << ip_.c_str() << ":" << port_;

    struct timeval begin;
    gettimeofday(&begin, nullptr);

    char buf[4096];
    std::string s("hello world!");

    parent_->setProgress(0);

    for (size_t i = 0; i < max_; i++) {
        int ret = acl_fiber_send(conn, s.c_str(), (int) s.size(), 0);
        if (ret <= 0) {
            qDebug() << "Send error: " << acl::last_serror();
            break;
        }

        ret = acl_fiber_recv(conn, buf, sizeof(buf) - 1, 0);
        if (ret <= 0) {
            qDebug() << "Fiber-" << acl::fiber::self() << "Recv error: " << acl::last_serror();
            break;
        }

        buf[ret] = 0;
        if (i < 10) {
            qDebug() << "Fiber-" << acl::fiber::self() << " recv: " << buf;
        }

        parent_->setProgress((100 * (int) i) / (int) max_);
        if (i % 10 == 0 && delay_ > 0) {
            acl::fiber::delay(delay_);
        }
    }

    parent_->setProgress(100);

    struct timeval end;
    gettimeofday(&end, nullptr);

    qDebug() << "All over, count=" << max_ << ", time cost=" << acl::stamp_sub(end, begin) << " ms\r\n";

    socket_close(conn);
    delete this;
}
