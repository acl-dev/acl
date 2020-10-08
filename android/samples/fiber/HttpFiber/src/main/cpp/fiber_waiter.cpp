//
// Created by shuxin 　　zheng on 2020/10/4.
//

#include "stdafx.h"
#include "log.h"
#include "fiber_waiter.h"

fiber_waiter::fiber_waiter(void) {
    box_ = new acl::fiber_tbox<acl::fiber>;
}

fiber_waiter::~fiber_waiter(void) {
    delete box_;
}

void fiber_waiter::push(acl::fiber* fiber) {
    box_->push(fiber);
}

void fiber_waiter::run(void) {
    log_info(">>>waiter fiber started!");

    // can't call any jni function in fiber.
    // test();

    while (true) {
        // blocking wait for one task;
        acl::fiber* fiber = box_->pop();
        fiber->start(12800000);
    }
}