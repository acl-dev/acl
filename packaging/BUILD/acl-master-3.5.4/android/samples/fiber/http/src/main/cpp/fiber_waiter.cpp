//
// Created by shuxin 　　zheng on 2020/10/4.
//

#include "stdafx.h"
#include "log.h"
#include "http_get.h"
#include "fiber_waiter.h"

fiber_waiter::fiber_waiter(void) {
    box_ = new acl::fiber_tbox<http_task>;
}

fiber_waiter::~fiber_waiter(void) {
    delete box_;
}

void fiber_waiter::push(http_task* t) {
    box_->push(t);
}

void fiber_waiter::run(void) {
    log_info(">>>waiter fiber started!");

    // can't call any jni function in fiber.
    // test();

    while (true) {
        // blocking wait for one task;
        http_task* t = box_->pop();

        // pop one http task and create one fiber to handle it
        go_stack(1024000)[=] {
            http_get request(t);
            request.run();
            t->done();
        };
    }
}