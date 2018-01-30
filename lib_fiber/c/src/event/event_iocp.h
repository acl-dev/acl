#ifndef EVENTS_IOCP_INCLUDE_H
#define EVENTS_IOCP_INCLUDE_H

#include "event.h"

#ifdef HAS_IOCP

#define MAX_WAIT_OBJECTS	64

EVENT *event_iocp_create(int setsize);

#endif

#endif
