//
//  FiberServer.cpp
//  fiber_server
//
//  Created by shuxin 　　zheng on 2020/9/26.
//  Copyright © 2020 acl. All rights reserved.
//

#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "fiber/libfiber.hpp"
#include "FiberClient.hpp"
#include "FiberServer.hpp"

FiberServer::FiberServer(const char* ip, int port)
: ip_(ip), port_(port), lfd_(-1) {}

int FiberServer::BindAndrListen(const char *ip, int port) {
     int fd;
     int on;
     struct sockaddr_in sa;

     memset(&sa, 0, sizeof(sa));
     sa.sin_family      = AF_INET;
     sa.sin_port        = htons(port);
     sa.sin_addr.s_addr = inet_addr(ip);

     fd = socket(AF_INET, SOCK_STREAM, 0);
     if (fd == -1) {
         printf("create socket error, %s\r\n", acl::fiber::last_serror());
         return -1;
     }

     if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))) {
         printf("setsockopt error %s\r\n", acl::fiber::last_serror());
         close(fd);
         return -1;
     }

     if (bind(fd, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
         printf("bind error %s\r\n", acl::fiber::last_serror());
         close(fd);
         return -1;
     }

     if (listen(fd, 1024) < 0) {
         printf("listen error %s\r\n", acl::fiber::last_serror());
         close(fd);
         return -1;
     }

     return fd;
}
    
void FiberServer::run(void) {
    lfd_ = BindAndrListen(ip_.c_str(), port_);
    if (lfd_ == -1) {
        return;
    }

    printf("listen %s:%d ok\r\n", ip_.c_str(), port_);

    while (true) {
        struct sockaddr_in sa;
        size_t len = sizeof(sa);
        int fd = accept(lfd_, (struct sockaddr *)& sa, (socklen_t *)& len);
        if (fd == -1) {
            printf("accept error\r\n");
            break;
        }
        printf("Accept on fd=%d\r\n", fd);
        FiberClient* fb = new FiberClient(fd);
        fb->start();
    }
    close(lfd_);
    delete this;
}
