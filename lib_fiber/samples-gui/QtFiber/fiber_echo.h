#ifndef FIBER_ECHO_H
#define FIBER_ECHO_H
#include "patch.h"

class fiber_echo : public acl::fiber {
public:
    fiber_echo(SOCKET conn) : conn_(conn) {}

protected:
    void run() override;

private:
    SOCKET conn_;

    ~fiber_echo();
};

#endif // FIBER_ECHO_H
