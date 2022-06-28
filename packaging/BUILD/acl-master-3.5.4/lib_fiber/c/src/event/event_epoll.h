#ifndef EVENT_EPOLL_INCLUDE_H
#define EVENT_EPOLL_INCLUDE_H

#include "event.h"

#ifdef HAS_EPOLL

EVENT *event_epoll_create(int setsize);

#endif

#endif
