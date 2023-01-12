#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber/fiber_cond.h"
#include "common/pthread_patch.h"
#include "fiber.h"

#include "sync_type.h"
#include "sync_timer.h"

typedef struct {
	RING head;
} THREAD_CONDS;

static pthread_once_t  __once_control = PTHREAD_ONCE_INIT;
static THREAD_CONDS   *__conds = NULL;
static pthread_mutex_t __lock;

static void free_conds_onexit(void)
{
	if (__conds) {
		mem_free(__conds);
		__conds = NULL;
	}
}

static void thread_once(void)
{
	pthread_mutex_init(&__lock, NULL);
	__conds = (THREAD_CONDS*) mem_malloc(sizeof(THREAD_CONDS));
	ring_init(&__conds->head);
	atexit(free_conds_onexit);
}

ACL_FIBER_COND *acl_fiber_cond_create(unsigned flag fiber_unused)
{
#ifdef SYS_UNIX
	pthread_mutexattr_t attr;
#endif
	ACL_FIBER_COND *cond;

	if (pthread_once(&__once_control, thread_once) != 0) {
		printf("%s(%d), %s: pthread_once error %s\r\n",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		abort();
	}

	cond = (ACL_FIBER_COND *) mem_calloc(1, sizeof(ACL_FIBER_COND));
	cond->waiters = array_create(10, ARRAY_F_UNORDER);
	ring_init(&cond->me);

#ifdef SYS_UNIX
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&cond->mutex, &attr);
	pthread_mutexattr_destroy(&attr);
#else
	pthread_mutex_init(&cond->mutex, NULL);
#endif

	pthread_mutex_lock(&__lock);
	ring_prepend(&__conds->head, &cond->me);
	pthread_mutex_unlock(&__lock);

	return cond;
}

void acl_fiber_cond_free(ACL_FIBER_COND *cond)
{
	pthread_mutex_lock(&__lock);
	ring_detach(&cond->me);
	pthread_mutex_unlock(&__lock);
	
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
	int n = acl_fiber_mutex_lock(l);  \
	if (n) {  \
		acl_fiber_set_error(n);  \
		msg_fatal("%s(%d), %s: acl_fiber_mutex_lock error=%s",  \
			__FILE__, __LINE__, __FUNCTION__, last_serror());  \
	}  \
} while (0)

#define	FIBER_UNLOCK(l) do {  \
	int n = acl_fiber_mutex_unlock(l);  \
	if (n) {  \
		acl_fiber_set_error(n);  \
		msg_fatal("%s(%d), %s: acl_fiber_mutex_unlock error=%s",  \
			__FILE__, __LINE__, __FUNCTION__, last_serror());  \
	}  \
} while (0)

static int fiber_cond_timedwait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex,
	int delay)
{
	EVENT *ev        = fiber_io_event();
	ACL_FIBER *fiber = acl_fiber_running();
	SYNC_OBJ *obj    = sync_obj_alloc(1);

	obj->type   = SYNC_OBJ_T_FIBER;
	obj->fb     = fiber;
	obj->delay  = delay;
	obj->status = 0;
	obj->cond   = cond;
	obj->timer  = sync_timer_get();

	LOCK_COND(cond);
	array_append(cond->waiters, obj);
	UNLOCK_COND(cond);

	if (delay >= 0) {
		// Add the current fiber to the timer which will be awakened
		// when the timer arrives before the fiber being awakened by
		// other fiber or thread.
		fiber_timer_add(obj->fb, obj->delay);
	}

	FIBER_UNLOCK(mutex);

	fiber->status = FIBER_STATUS_WAIT_COND;
	WAITER_INC(ev);

	// Hang the current fiber and will wakeup if the timer arrives or
	// be awakened by the other fiber or thread.
	acl_fiber_switch();

	WAITER_DEC(ev);

	FIBER_LOCK(mutex);

	LOCK_COND(cond);
	array_delete_obj(cond->waiters, obj, NULL);
	UNLOCK_COND(cond);

	if (fiber->flag & FIBER_F_TIMER) {
		// The obj has been deleted in sync_timer.c when timeout.
		fiber->flag &= ~FIBER_F_TIMER;
		sync_obj_unrefer(obj);
		return FIBER_ETIME;
	}

	sync_obj_unrefer(obj);
	return 0;
}

static int thread_cond_timedwait(ACL_FIBER_COND *cond, ACL_FIBER_MUTEX *mutex,
	int delay)
{
	// Create one waiting object with the one reference added, which can
	// be used by multiple threads, and will be released really when its
	// refernece is zero.
	SYNC_OBJ *obj = sync_obj_alloc(1);

	obj->type = SYNC_OBJ_T_THREAD;
	obj->base = fbase_alloc(0);
	obj->tid  = thread_self();

	// The in/out fds opened by fbase_event_open() will be closed
	// in sync_obj_unrefer().
	fbase_event_open(obj->base);

	LOCK_COND(cond);
	array_append(cond->waiters, obj);
	UNLOCK_COND(cond);

	FIBER_UNLOCK(mutex);

	if (delay >= 0 && read_wait(obj->base->event_in, delay) == -1) {
		LOCK_COND(cond);
		array_delete_obj(cond->waiters, obj, NULL);
		UNLOCK_COND(cond);

		FIBER_LOCK(mutex);
		sync_obj_unrefer(obj);
		return FIBER_ETIME;
	}

	if (fbase_event_wait(obj->base) == -1) {
		msg_error("%s(%d), %s: wait event error",
			__FILE__, __LINE__, __FUNCTION__);

		LOCK_COND(cond);
		array_delete_obj(cond->waiters, obj, NULL);
		UNLOCK_COND(cond);

		FIBER_LOCK(mutex);
		sync_obj_unrefer(obj);
		return FIBER_EINVAL;
	}

	FIBER_LOCK(mutex);
	sync_obj_unrefer(obj);
	return 0;
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

	obj = (SYNC_OBJ*) array_head(cond->waiters);
	if (obj == NULL) {
		UNLOCK_COND(cond);
		return 0;
	}

	switch (obj->type) {
	case SYNC_OBJ_T_FIBER:
	case SYNC_OBJ_T_THREAD:
		sync_obj_refer(obj);
		(void) array_pop_front(cond->waiters);
		break;
	default:
		msg_fatal("%s: unknown type=%d", __FUNCTION__, obj->type);
		break;
	}

	UNLOCK_COND(cond);

	// If the waiter is a fiber, we should use sync_timer_wakeup() to
	// notify the fiber, or if it's a thread, we should use the
	// fbase_event_wakeup() to wakeup it.
	// That is to say, a fiber waiter is managed by the sync_timer, and
	// the thread waiter uses a temporary IO to wait for a notice.
	if (obj->type == SYNC_OBJ_T_FIBER) {
		sync_timer_wakeup(obj->timer, obj);
	}
	// else: obj->type == SYNC_OBJ_T_THREAD
	else if (var_hook_sys_api) {
		socket_t out = obj->base->event_out;
		// The waiter is a thread, the out fd is temporaryly
		// created by the thread waiter, so we just use one
		// temporary FILE_EVENT to bind the out fd, and
		// release it after notify the waiter thread.
		FILE_EVENT *fe = fiber_file_cache_get(out);
		fe->mask |= EVENT_SYSIO;
		ret = fbase_event_wakeup(obj->base);
		fiber_file_cache_put(fe);
	} else {
		ret = fbase_event_wakeup(obj->base);
	}

	// Unrefer the waiter object, which will be really freed when its
	// reference is zero. It's safely that the waiter object is used
	// by multiple threads with using the reference way.
	sync_obj_unrefer(obj);
	return ret;
}
