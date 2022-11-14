#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber/fiber_cond.h"
#include "common/pthread_patch.h"
#include "fiber.h"

#include "sync_type.h"
#include "sync_timer.h"

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

	pthread_cond_init(&cond->thread_cond, NULL);

	return cond;
}

void acl_fiber_cond_free(ACL_FIBER_COND *cond)
{
	array_free(cond->waiters, NULL);
	pthread_mutex_destroy(&cond->mutex);
	pthread_cond_destroy(&cond->thread_cond);
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

static int fiber_cond_timedwait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex,
	int delay_ms)
{
	EVENT *ev        = fiber_io_event();
	ACL_FIBER *fiber = acl_fiber_running();
	SYNC_OBJ *obj    = (SYNC_OBJ*) mem_malloc(sizeof(SYNC_OBJ));

	obj->type   = SYNC_OBJ_T_FIBER;
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

static int thread_cond_timedwait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex,
	int delay_ms)
{
	SYNC_OBJ *obj = (SYNC_OBJ*) mem_calloc(1, sizeof(SYNC_OBJ));
	struct timespec timeout;
	struct timeval  tv;
	int ret;

	obj->type = SYNC_OBJ_T_THREAD;

	LOCK_COND(cond);
	array_append(cond->waiters, obj);
	UNLOCK_COND(cond);

	gettimeofday(&tv, NULL);
	timeout.tv_sec = tv.tv_sec + delay_ms / 1000;
	timeout.tv_nsec = tv.tv_usec * 1000 + (delay_ms % 1000) * 1000 * 1000;

	ret = pthread_cond_timedwait(&cond->thread_cond,
			&mutex->thread_lock, &timeout);
	LOCK_COND(cond);
	// If the object hasn't been poped by producer, we should remove it
	// from the waiters' array and free it here.
	if (array_delete_obj(cond->waiters, obj, NULL) == 0) {
		mem_free(obj);
	}
	// else: the object must be poped by acl_fiber_cond_signal, and will
	// be freed there.
	UNLOCK_COND(cond);

	return ret;
}

int acl_fiber_cond_timedwait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex,
	int delay_ms)
{
	if (var_hook_sys_api) {
		return fiber_cond_timedwait(cond, mutex, delay_ms);
	} else {
		return thread_cond_timedwait(cond, mutex, delay_ms);
	}
}

int acl_fiber_cond_wait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex)
{
	return acl_fiber_cond_timedwait(cond, mutex, -1);
}

int acl_fiber_cond_signal(ACL_FIBER_COND *cond)
{
	SYNC_OBJ *obj;
	int ret = 0;

	LOCK_COND(cond);
	obj = array_pop_front(cond->waiters);
	if (obj == NULL) {
		UNLOCK_COND(cond);
		return 0;
	}

	if (obj->type == SYNC_OBJ_T_FIBER) {
		sync_timer_wakeup(obj->timer, obj);
	} else if (obj->type == SYNC_OBJ_T_THREAD) {
		ret = pthread_cond_signal(&obj->cond->thread_cond);
		mem_free(obj);
	} else {
		msg_fatal("%s: unknown type=%d", __FUNCTION__, obj->type);
	}

	UNLOCK_COND(cond);
	return ret;
}

int fiber_cond_delete_waiter(ACL_FIBER_COND *cond, SYNC_OBJ *obj)
{
	int ret;

	LOCK_COND(cond);
	ret = array_delete_obj(cond->waiters, obj, NULL);
	UNLOCK_COND(cond);
	return ret;
}
