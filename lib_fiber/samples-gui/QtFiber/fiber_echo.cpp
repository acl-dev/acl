#include "stdafx.h"
#include "patch.h"
#include "fiber_echo.h"

fiber_echo::~fiber_echo() {
    socket_close(conn_);
}

void fiber_echo::run() {
    char buf[8192];
    while (true) {
        int ret = acl_fiber_recv(conn_, buf, sizeof(buf) - 1, 0);
        if (ret == -1) {
            break;
        }

        buf[ret] = 0;
        if (acl_fiber_send(conn_, buf, ret, 0) != ret) {
            break;
        }
    }

    delete this;
}
