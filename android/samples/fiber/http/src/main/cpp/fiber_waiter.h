//
// Created by shuxin 　　zheng on 2020/10/4.
//

#pragma once

#include "http_task.h"

class fiber_waiter : public acl::fiber {
public:
    fiber_waiter(void);
    ~fiber_waiter(void);

    // ass one http request to the fiber queue
    void push(http_task* t);

protected:
    // @override
    void run(void);

private:
    acl::fiber_tbox<http_task>* box_;
};
