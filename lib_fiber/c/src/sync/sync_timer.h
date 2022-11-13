#ifndef	__SYNC_TIMER_INCLUDE_H__
#define	__SYNC_TIMER_INCLUDE_H__

typedef struct SYNC_TIMER SYNC_TIMER;
typedef struct ACL_FIBER ACL_FIBER;

typedef struct {
	SYNC_TIMER *timer;
	ACL_FIBER *fb;
	int delay;
	int status;
} SYNC_OBJ;

SYNC_TIMER *sync_timer_get(void);
void sync_timer_add(SYNC_TIMER *waiter, SYNC_OBJ *obj);
void sync_timer_wakeup(SYNC_TIMER *waiter, SYNC_OBJ *obj);

#endif
