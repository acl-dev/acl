#ifndef	__SYNC_WAITER_INCLUDE_H__
#define	__SYNC_WAITER_INCLUDE_H__

typedef struct SYNC_WAITER SYNC_WAITER;
typedef struct ACL_FIBER ACL_FIBER;

// In sync_waiter.c
SYNC_WAITER *sync_waiter_get(void);
void sync_waiter_wakeup(SYNC_WAITER *waiter, ACL_FIBER *fb);

#endif
