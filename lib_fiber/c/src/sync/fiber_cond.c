#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber/fiber_cond.h"
#include "common/pthread_patch.h"
#include "fiber.h"
#include "sync_timer.h"

struct ACL_FIBER_COND {
	ARRAY          *waiters;
	pthread_mutex_t mutex;
};

ACL_FIBER_COND *acl_fiber_cond_create(unsigned flag fiber_unused)
{
#ifdef SYS_UNIX
	pthread_mutexattr_t attr;
#endif
	ACL_FIBER_COND *cond = (ACL_FIBER_COND *)
		mem_calloc(1, sizeof(ACL_FIBER_COND));

	cond->waiters = array_create(10, ARRAY_F_UNORDER);

#ifdef SYS_UNIX
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&cond->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
#else
	pthread_mutex_init(&cond->mutex, NULL);
#endif

	return cond;
}

void acl_fiber_cond_free(ACL_FIBER_COND *cond)
{
	array_free(cond->waiters, NULL);
	pthread_mutex_destroy(&cond->mutex);
	mem_free(cond);
}

#define	LOCK_COND(c) do {  \
	int n = pthread_mutex_lock(&(c)->mutex);  \
	if (n) {  \
		acl_fiber_set_error(n);  \
		msg_fatal("%s(%d), %s: pthread_mutex_lock error=%s",  \
			__FILE__, __LINE__, __FUNCTION__, last_serror());  \
	}  \
} while (0)

#define	UNLOCK_COND(c) do {  \
	int n = pthread_mutex_unlock(&(c)->mutex);  \
	if (n) {  \
		acl_fiber_set_error(n);  \
		msg_fatal("%s(%d), %s: pthread_mutex_unlock error=%s",  \
			__FILE__, __LINE__, __FUNCTION__, last_serror());  \
	}  \
} while (0)

#define	FIBER_LOCK(l) do {  \
	if (acl_fiber_mutex_lock(l) != 0) {  \
		msg_fatal("acl_fiber_mutex_lock failed");  \
	}  \
} while (0)

#define	FIBER_UNLOCK(l) do {  \
	if (acl_fiber_mutex_unlock(l) != 0) {  \
		msg_fatal("acl_fiber_mutex_unlock failed");  \
	}  \
} while (0)

int acl_fiber_cond_timedwait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex,
	int delay_ms)
{
	EVENT *ev        = fiber_io_event();
	ACL_FIBER *fiber = acl_fiber_running();
	SYNC_OBJ *obj    = (SYNC_OBJ*) mem_calloc(1, sizeof(SYNC_OBJ));

	obj->fb     = fiber;
	obj->delay  = delay_ms;
	obj->status = 0;
	obj->cond   = cond;
	obj->timer  = sync_timer_get();

	LOCK_COND(cond);
	array_append(cond->waiters, obj);
	sync_timer_await(obj->timer, obj);
	UNLOCK_COND(cond);

	FIBER_UNLOCK(mutex);

	ev->waiter++;
	acl_fiber_switch();
	ev->waiter--;

	FIBER_LOCK(mutex);

	LOCK_COND(cond);
	if (obj->status & SYNC_STATUS_TIMEOUT) {
		// The obj has been deleted in sync_timer.c when timeout.
		mem_free(obj);
		UNLOCK_COND(cond);
		return FIBER_ETIME;
	}

	mem_free(obj);
	UNLOCK_COND(cond);
	return 0;
}

int acl_fiber_cond_wait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex)
{
	return acl_fiber_cond_timedwait(cond, mutex, -1);
}

int acl_fiber_cond_signal(ACL_FIBER_COND *cond)
{
	SYNC_OBJ *obj;

	LOCK_COND(cond);
	obj = array_pop_front(cond->waiters);
	if (obj) {
		sync_timer_wakeup(obj->timer, obj);
	}
	UNLOCK_COND(cond);
	return 0;
}

int fiber_cond_delete_waiter(ACL_FIBER_COND *cond, SYNC_OBJ *obj)
{
	int ret;

	LOCK_COND(cond);
	ret = array_delete_obj(cond->waiters, obj, NULL);
	UNLOCK_COND(cond);
	return ret;
}
