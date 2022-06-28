#include "stdafx.h"
#include <pthread.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#include <sys/eventfd.h>
#endif
#include "fiber/lib_fiber.h"
#include "fiber.h"

//#define	USE_THREAD_MUTEX

struct ACL_FIBER_EVENT {
	ACL_RING      me;
	acl_pthread_t tid;
	FIBER_BASE   *owner;
	ACL_ATOMIC   *atomic;
	long long     value;
#ifdef	USE_THREAD_MUTEX
	acl_pthread_mutex_t lock;
#else
	ACL_ATOMIC *lock;
	long long   lock_value;
#endif
	ACL_RING    waiters;
};

ACL_FIBER_EVENT *acl_fiber_event_create(void)
{
#ifdef	USE_THREAD_MUTEX
	pthread_mutexattr_t attr;
#endif
	ACL_FIBER_EVENT *event = (ACL_FIBER_EVENT *)
		acl_mymalloc(sizeof(ACL_FIBER_EVENT));

	acl_ring_init(&event->me);
	event->owner = NULL;
	event->tid   = 0;

	event->atomic = acl_atomic_new();
	acl_atomic_set(event->atomic, &event->value);
	acl_atomic_int64_set(event->atomic, 0);

#ifdef	USE_THREAD_MUTEX
	pthread_mutexattr_init(&attr);
	acl_pthread_mutex_init(&event->lock, &attr);
	pthread_mutexattr_destroy(&attr);
#else
	event->lock = acl_atomic_new();
	acl_atomic_set(event->lock, &event->lock_value);
	acl_atomic_int64_set(event->lock, 0);
#endif

	acl_ring_init(&event->waiters);

	return event;
}

void acl_fiber_event_free(ACL_FIBER_EVENT *event)
{
	acl_atomic_free(event->atomic);
#ifdef	USE_THREAD_MUTEX
	acl_pthread_mutex_destroy(&event->lock);
#else
	acl_atomic_free(event->lock);
#endif
	acl_myfree(event);
}

static inline void channel_open(FIBER_BASE *fbase)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
	int flags = 0;
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	flags |= FD_CLOEXEC;
# endif
	if (fbase->mutex_in == -1) {
		fbase->mutex_in  = eventfd(0, flags);
		fbase->mutex_out = fbase->mutex_in;
	}
#else
	int fds[2];

	if (fbase->mutex_in >= 0) {
		assert(fbase->mutex_out >= 0);
		return;
	}

	if (acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0)
		acl_msg_fatal("%s(%d), %s: acl_duplex_pipe error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	fbase->mutex_in  = fds[0];
	fbase->mutex_out = fds[1];
#endif
}

void fbase_event_close(FIBER_BASE *fbase)
{
	if (fbase->mutex_in >= 0)
		close(fbase->mutex_in);
	if (fbase->mutex_out != fbase->mutex_in && fbase->mutex_out >= 0)
		close(fbase->mutex_out);
	fbase->mutex_in  = -1;
	fbase->mutex_out = -1;
	acl_atomic_int64_set(fbase->atomic, 0);
}

int fbase_event_wait(FIBER_BASE *fbase)
{
	long long n;

	assert(fbase->mutex_in >= 0);
	if (read(fbase->mutex_in, &n, sizeof(n)) != sizeof(n)) {
		acl_msg_error("%s(%d), %s: read error %s, in=%d",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror(),
			fbase->mutex_in);
		return -1;
	}
	if (acl_atomic_int64_cas(fbase->atomic, 1, 0) != 1)
		acl_msg_fatal("%s(%d), %s: atomic corrupt",
			__FILE__, __LINE__, __FUNCTION__);
	return 0;
}

int fbase_event_wakeup(FIBER_BASE *fbase)
{
	long long n = 1;

	if (LIKELY(acl_atomic_int64_cas(fbase->atomic, 0, 1) != 0))
		return 0;

	assert(fbase->mutex_out >= 0);
	if (write(fbase->mutex_out, &n, sizeof(n)) != sizeof(n)) {
		acl_msg_error("%s(%d), %s: write error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}
	return 0;
}

static inline void __ll_lock(ACL_FIBER_EVENT *event)
{
#ifdef	USE_THREAD_MUTEX
	acl_pthread_mutex_lock(&event->lock);
#else
	while (acl_atomic_int64_cas(event->lock, 0, 1) != 0) {}
#endif
}

static inline void __ll_unlock(ACL_FIBER_EVENT *event)
{
#ifdef	USE_THREAD_MUTEX
	acl_pthread_mutex_unlock(&event->lock);
#else
	if (acl_atomic_int64_cas(event->lock, 1, 0) != 1)
		acl_msg_fatal("%s(%d), %s: lock corrupt",
			__FILE__, __LINE__, __FUNCTION__);
#endif
}

int acl_fiber_event_wait(ACL_FIBER_EVENT *event)
{
	ACL_FIBER  *fiber = acl_fiber_running();
	FIBER_BASE *fbase;

	if (LIKELY(acl_atomic_int64_cas(event->atomic, 0, 1) == 0)) {
		event->owner = fiber ? &fiber->base : NULL;
		event->tid   = acl_pthread_self();
		return 0;
	}

	// FIBER_BASE obj will be created if is not in fiber scheduled
	fbase = fiber ? &fiber->base : fbase_alloc();

	channel_open(fbase);

	__ll_lock(event);
	acl_ring_prepend(&event->waiters, &fbase->mutex_waiter);
	__ll_unlock(event);

	while (1) {
		if (acl_atomic_int64_cas(event->atomic, 0, 1) == 0) {
			__ll_lock(event);
			acl_ring_detach(&fbase->mutex_waiter);
			__ll_unlock(event);

			event->owner = fbase;
			event->tid   = acl_pthread_self();
			break;
		}

		if (fbase_event_wait(fbase) == -1) {
			fbase_event_close(fbase);
			if (fbase->flag & FBASE_F_BASE)
				fbase_free(fbase);

			acl_msg_error("%s(%d), %s: event wait error %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_last_serror());
			return -1;
		}
	}

	fbase_event_close(fbase);
	if (fbase->flag & FBASE_F_BASE) {
		event->owner = NULL;
		fbase_free(fbase);
	}
	return 0;
}

int acl_fiber_event_trywait(ACL_FIBER_EVENT *event)
{
	if (acl_atomic_int64_cas(event->atomic, 0, 1) == 0) {
		ACL_FIBER *fiber = acl_fiber_running();
		event->owner     = fiber ? &fiber->base : NULL;
		event->tid       = acl_pthread_self();
		return 0;
	}
	return -1;
}

#define RING_TO_FIBER(r) \
    ((FIBER_BASE *) ((char *) (r) - offsetof(FIBER_BASE, mutex_waiter)))

#define FIRST_FIBER(head) \
    (acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

int acl_fiber_event_notify(ACL_FIBER_EVENT *event)
{
	ACL_FIBER  *curr  = acl_fiber_running();
	FIBER_BASE *owner = curr ? &curr->base : NULL, *waiter;

	if (UNLIKELY(event->owner != owner)) {
		acl_msg_error("%s(%d), %s: fiber(%p) is not the owner(%p)",
			__FILE__, __LINE__, __FUNCTION__, owner, event->owner);
		return -1;
	} else if (UNLIKELY(event->owner == NULL
		&& event->tid != acl_pthread_self())) {

		acl_msg_error("%s(%d), %s: tid(%ld) is not the owner(%ld)",
			__FILE__, __LINE__, __FUNCTION__,
			event->tid, acl_pthread_self());
		return -1;
	}

	if (acl_atomic_int64_cas(event->atomic, 1, 0) != 1)
		acl_msg_fatal("%s(%d), %s: atomic corrupt",
			__FILE__, __LINE__, __FUNCTION__);

	__ll_lock(event);
	waiter = FIRST_FIBER(&event->waiters);
	if (waiter && fbase_event_wakeup(waiter) == -1) {
		__ll_unlock(event);
		return -1;
	}
	__ll_unlock(event);

	return 0;
}
