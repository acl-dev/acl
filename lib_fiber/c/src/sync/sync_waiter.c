#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"
#include "common/mbox.h"
#include "sync_waiter.h"

#define	USE_MBOX

struct SYNC_WAITER {
	pthread_mutex_t lock;
	ARRAY *waiters;
	ARRAY *ready;
	ACL_FIBER *fb;
	ATOMIC *atomic;
	long long value;
#ifdef	USE_MBOX
	MBOX *box;
#endif
	int left;
	int stop;
};

static SYNC_WAITER *sync_waiter_new(void)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) mem_calloc(1, sizeof(SYNC_WAITER));

	pthread_mutex_init(&waiter->lock, NULL);
	waiter->waiters = array_create(100, ARRAY_F_UNORDER);
	waiter->ready   = array_create(100, ARRAY_F_UNORDER);
	waiter->atomic  = atomic_new();
	atomic_set(waiter->atomic, &waiter->value);
	atomic_int64_set(waiter->atomic, 0);

#ifdef	USE_MBOX
	waiter->box = mbox_create(MBOX_T_MPSC);
#endif
	waiter->left = 0;

	return waiter;
}

static void sync_waiter_free(SYNC_WAITER *waiter)
{
	pthread_mutex_destroy(&waiter->lock);
	atomic_free(waiter->atomic);
	array_free(waiter->waiters, NULL);
	array_free(waiter->ready, NULL);
#ifdef	USE_MBOX
	mbox_free(waiter->box, NULL);
#endif
	mem_free(waiter);
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;
static pthread_key_t  __waiter_key;

static void thread_free(void *ctx)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) ctx;
	sync_waiter_free(waiter);
}

static void thread_init(void)
{
	if (pthread_key_create(&__waiter_key, thread_free) != 0) {
		abort();
	}
}

/*
static void check_timedout(SYNC_WAITER *waiter)
{
	(void) waiter;
}
*/

#ifndef	USE_MBOX
static void wakeup_waiters(SYNC_WAITER *waiter)
{
	ACL_FIBER *fb;
	int n = 0;

	pthread_mutex_lock(&waiter->lock);
	while ((fb = (ACL_FIBER*) array_pop_front(waiter->ready)) != NULL) {
		acl_fiber_ready(fb);
		n++;
	}
	pthread_mutex_unlock(&waiter->lock);

#if 0
	if (n == 0) {
		printf(">>>>thread-%lu n=%d\n", pthread_self(), n);
	}
#endif
}
#endif

static __thread int __cnt = 0;
static void fiber_waiting(ACL_FIBER *fiber, void *ctx)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) ctx;
	int delay = -1;

	while (!waiter->stop) {
#ifdef	USE_MBOX
		int res;
		ACL_FIBER *fb = mbox_read(waiter->box, delay, &res);
		if (fb) {
			int left;
			__cnt++;
			pthread_mutex_lock(&waiter->lock);
			left = --waiter->left;
			pthread_mutex_unlock(&waiter->lock);
			printf(">>>thread-%lu, read one fb=%p, cnt=%d, fb status=%d, left=%d\n",
				pthread_self(), fb, __cnt, fb->status, left);
			assert(fb->status == FIBER_STATUS_SUSPEND);
			acl_fiber_ready(fb);
		} else {
			assert(0);
		}
#else
		if (read_wait(fiber->base.event_in, delay) == -1) {
			check_timedout(waiter);
			//wakeup_waiters(waiter);
			continue;
		}

		if (fbase_event_wait(&fiber->base) == -1) {
			abort();
		}
		wakeup_waiters(waiter);
#endif

#if 0
		if (atomic_int64_cas(waiter->atomic, 1, 0) != 1) {
			assert(0);
		}
#endif
	}

	printf(">>>begin free base=%p\n", &fiber->base);
#ifndef	USE_MBOX
	fbase_event_close(&fiber->base);
#endif
}


SYNC_WAITER *sync_waiter_get(void)
{
	SYNC_WAITER *waiter;

	if (pthread_once(&__once_control, thread_init) != 0) {
		abort();
	}

	waiter = (SYNC_WAITER*) pthread_getspecific(__waiter_key);
	if (waiter == NULL) {
		waiter = sync_waiter_new();
		pthread_setspecific(__waiter_key, waiter);
		waiter->fb = acl_fiber_create(fiber_waiting, waiter, 320000);
#ifndef	USE_MBOX
		fbase_event_open(&waiter->fb->base);
#endif
	}

#ifndef	USE_MBOX
	assert(waiter->fb->base.event_out != INVALID_SOCKET);
#endif
	return waiter;
}

void sync_waiter_append(SYNC_WAITER *waiter, ACL_FIBER *fb)
{
	pthread_mutex_lock(&waiter->lock);
	array_append(waiter->waiters, fb);
	pthread_mutex_unlock(&waiter->lock);
}

void sync_waiter_wakeup(SYNC_WAITER *waiter, ACL_FIBER *fb)
{
#ifdef	USE_MBOX
	pthread_mutex_lock(&waiter->lock);
	array_delete_obj(waiter->waiters, fb, NULL);
	waiter->left++;
	printf("thread-%lu >>>wakeup left=%d, fb=%p, status=%d\n",
		pthread_self(), waiter->left, fb, fb->status);
	pthread_mutex_unlock(&waiter->lock);
	mbox_send(waiter->box, fb);
#else
	pthread_mutex_lock(&waiter->lock);
	array_delete_obj(waiter->waiters, fb, NULL);
	array_append(waiter->waiters, fb);
	waiter->left++;
	pthread_mutex_unlock(&waiter->lock);

	// Only one wakeup action can be executed in the same time.
#if 0
	if (atomic_int64_cas(waiter->atomic, 0, 1) != 0) {
		return;
	}
#endif

	// Notify the waiter fiber to handle the ready fibers.
	fbase_event_wakeup(&waiter->fb->base);
#endif

}
