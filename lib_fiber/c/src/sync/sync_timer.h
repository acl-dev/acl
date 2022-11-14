#ifndef	__SYNC_TIMER_INCLUDE_H__
#define	__SYNC_TIMER_INCLUDE_H__

typedef struct SYNC_TIMER SYNC_TIMER;
typedef struct ACL_FIBER ACL_FIBER;
typedef struct ACL_FIBER_COND ACL_FIBER_COND;

typedef struct SYNC_OBJ {
	RING me;
	SYNC_TIMER *timer;
	ACL_FIBER *fb;
	ACL_FIBER_COND *cond;

	int type;
#define	SYNC_OBJ_T_FIBER	1
#define	SYNC_OBJ_T_THREAD	2

	long long expire;
	int delay;

	int status;
#define	SYNC_STATUS_TIMEOUT	(1 << 0)
#define	SYNC_STATUS_DELAYED	(1 << 1)
} SYNC_OBJ;

typedef struct SYNC_MSG {
	SYNC_OBJ *obj;

	int action;
#define	SYNC_ACTION_AWAIT	1
#define	SYNC_ACTION_WAKEUP	2
} SYNC_MSG;

// In sync_timer.c
SYNC_TIMER *sync_timer_get(void);
void sync_timer_await(SYNC_TIMER *waiter, SYNC_OBJ *obj);
void sync_timer_wakeup(SYNC_TIMER *waiter, SYNC_OBJ *obj);

// In fiber_cond.c
int  fiber_cond_delete_waiter(ACL_FIBER_COND *cond, SYNC_OBJ *obj);

#endif
