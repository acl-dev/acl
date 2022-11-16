#ifndef	__SYNC_TIMER_INCLUDE_H__
#define	__SYNC_TIMER_INCLUDE_H__

#include "sync_type.h"

// In sync_timer.c
SYNC_TIMER *sync_timer_get(void);
void sync_timer_await(SYNC_TIMER *waiter, SYNC_OBJ *obj);
void sync_timer_wakeup(SYNC_TIMER *waiter, SYNC_OBJ *obj);

// In fiber_cond.c
int  fiber_cond_delete_waiter(ACL_FIBER_COND *cond, SYNC_OBJ *obj);

#endif
