//
// Created by shuxin 　　zheng on 2020/10/4.
//

#pragma once

class fiber_waiter : public acl::fiber {
public:
    fiber_waiter(void);
    ~fiber_waiter(void);

    // add the fiber queue
    void push(acl::fiber* fiber);

protected:
    // @override
    void run(void);

private:
    acl::fiber_tbox<acl::fiber>* box_;
};
