#ifndef EVENT_SELECT_INCLUDE_H
#define EVENT_SELECT_INCLUDE_H

#include "event.h"

#ifdef HAS_SELECT

EVENT *event_select_create(int setsize);

#endif

#endif
