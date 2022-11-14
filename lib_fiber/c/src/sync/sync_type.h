#ifndef	__SYNC_TYPE_INCLUDE_H__
#define	__SYNC_TYPE_INCLUDE_H__

struct ACL_FIBER_MUTEX {
	unsigned flags;
	ATOMIC *atomic;
	long long value;
	ARRAY  *waiters;
	pthread_mutex_t lock;
	pthread_mutex_t thread_lock;
};

struct ACL_FIBER_COND {
	ARRAY          *waiters;
	pthread_mutex_t mutex;
	pthread_cond_t  thread_cond;
};

#endif
