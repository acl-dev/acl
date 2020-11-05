//
//  FiberTest.cpp
//  fiber_server
//
//  Created by shuxin 　　zheng on 2020/9/26.
//  Copyright © 2020 acl. All rights reserved.
//

#include <thread>
#include "fiber/libfiber.hpp"
#include "FiberServer.hpp"
#include "FiberThread.hpp"

static void ThreadRun(void) {
    const char* ip = "127.0.0.1";
    int port = 8192;
    FiberServer* fb = new FiberServer(ip, port);
    fb->start();
    
    acl::fiber::schedule();
}

void StartThread(void) {
    std::thread* thread = new std::thread(ThreadRun);
    thread->detach();
}
