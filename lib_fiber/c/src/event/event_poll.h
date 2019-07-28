#ifndef EVENT_POLL_INCLUDE_H
#define EVENT_POLL_INCLUDE_H

#include "event.h"

#ifdef HAS_POLL

EVENT *event_poll_create(int setsize);

#endif

#endif
