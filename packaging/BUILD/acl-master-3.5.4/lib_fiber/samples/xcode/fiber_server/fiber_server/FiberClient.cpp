//
//  FiberClient.cpp
//  fiber_server
//
//  Created by shuxin 　　zheng on 2020/9/26.
//  Copyright © 2020 acl. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include "fiber/libfiber.hpp"
#include "FiberClient.hpp"

void FiberClient::run(void) {
    printf("FiberClient run, fd=%d\r\n", fd_);
    char buf[8192];
    while (true) {
        ssize_t ret = read(fd_, buf, sizeof(buf));
        if (ret <= 0) {
            break;
        }
        if (write(fd_, buf, ret) <= 0) {
            break;
        }
    }
    printf("Close client fd=%d\r\n", fd_);
    close(fd_);
    delete this;
}
