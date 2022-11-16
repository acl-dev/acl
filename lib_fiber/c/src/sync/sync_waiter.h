#ifndef	__SYNC_WAITER_INCLUDE_H__
#define	__SYNC_WAITER_INCLUDE_H__

#include "sync_type.h"

// In sync_waiter.c
SYNC_WAITER *sync_waiter_get(void);
void sync_waiter_wakeup(SYNC_WAITER *waiter, ACL_FIBER *fb);

#endif
