#ifndef EVENT_IO_URING_INCLUDE_H
#define EVENT_IO_URING_INCLUDE_H

#include "event.h"

#ifdef HAS_EPOLL

EVENT *event_io_uring_create(int size);

#endif

#endif
