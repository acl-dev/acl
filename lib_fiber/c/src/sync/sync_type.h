#ifndef	__SYNC_TYPE_INCLUDE_H__
#define	__SYNC_TYPE_INCLUDE_H__

typedef struct SYNC_TIMER SYNC_TIMER;
typedef struct SYNC_WAITER SYNC_WAITER;

struct ACL_FIBER_MUTEX {
	RING me;
	long owner;
	ACL_FIBER *fiber;
	unsigned flags;
	ARRAY  *waiters;
	ARRAY  *waiting_threads;
	pthread_mutex_t lock;
	pthread_mutex_t thread_lock;
};

struct ACL_FIBER_COND {
	RING   me;
	ARRAY *waiters;
	pthread_mutex_t mutex;
};

typedef struct SYNC_OBJ {
	RING me;
	SYNC_TIMER *timer;
	ACL_FIBER *fb;
	ACL_FIBER_COND *cond;

	int type;
#define	SYNC_OBJ_T_FIBER	1
#define	SYNC_OBJ_T_THREAD	2

	long tid;
	FIBER_BASE *base;
	ATOMIC *atomic;
	long long atomic_value;

	long long expire;
	int delay;

	int status;
#define	SYNC_STATUS_TIMEOUT	(1 << 0)
} SYNC_OBJ;

typedef struct SYNC_MSG {
	SYNC_OBJ *obj;

	int action;
#define	SYNC_ACTION_AWAIT	1
#define	SYNC_ACTION_WAKEUP	2
} SYNC_MSG;

SYNC_OBJ *sync_obj_alloc(int shared);
unsigned sync_obj_refer(SYNC_OBJ *obj);
unsigned sync_obj_unrefer(SYNC_OBJ *obj);

#endif
