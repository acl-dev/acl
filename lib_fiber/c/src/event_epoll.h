#ifndef EVENT_EPOLL_INCLUDE_H
#define EVENT_EPOLL_INCLUDE_H

#include "event.h"

void hook_epoll(void);
EVENT *event_epoll_create(int setsize);

#endif
