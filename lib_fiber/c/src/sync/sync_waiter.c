#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"
#include "sync_waiter.h"

struct SYNC_WAITER {
	pthread_mutex_t lock;
	RING waiters;
	RING ready;
	ACL_FIBER *fb;
	ATOMIC *atomic;
	long long value;
	int stop;
};

static SYNC_WAITER *sync_waiter_new(void)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) mem_calloc(1, sizeof(SYNC_WAITER));

	pthread_mutex_init(&waiter->lock, NULL);
	ring_init(&waiter->waiters);
	ring_init(&waiter->ready);
	waiter->atomic  = atomic_new();
	atomic_set(waiter->atomic, &waiter->value);
	atomic_int64_set(waiter->atomic, 0);

	return waiter;
}

static void sync_waiter_free(SYNC_WAITER *waiter)
{
	pthread_mutex_destroy(&waiter->lock);
	atomic_free(waiter->atomic);
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

static void check_timedout(SYNC_WAITER *waiter)
{
	(void) waiter;
}

static void wakeup_waiters(SYNC_WAITER *waiter)
{
	RING *head;

	while ((head = ring_pop_head(&waiter->ready)) != NULL) {
		ACL_FIBER *fiber = RING_TO_APPL(head, ACL_FIBER, me);
		acl_fiber_ready(fiber);
	}
}

static void fiber_waiting(ACL_FIBER *fb, void *ctx)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) ctx;
	int delay = 1000;

	waiter->fb = fb;
	fbase_event_open(&fb->base);

	while (!waiter->stop) {
		if (read_wait(fb->base.event_in, delay) == -1) {
			check_timedout(waiter);
			continue;
		}

		if (fbase_event_wait(&fb->base) == -1) {
			abort();
		}

		wakeup_waiters(waiter);
	}

	fbase_event_close(&fb->base);
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
		acl_fiber_create(fiber_waiting, waiter, 320000);
	}

	return waiter;
}

void sync_waiter_append(SYNC_WAITER *waiter, ACL_FIBER *fb)
{
	pthread_mutex_lock(&waiter->lock);
	ring_prepend(&waiter->waiters, &fb->me);
	pthread_mutex_unlock(&waiter->lock);
}

void sync_waiter_wakeup(SYNC_WAITER *waiter, ACL_FIBER *fb)
{
	pthread_mutex_lock(&waiter->lock);
	ring_detach(&fb->me);
	ring_prepend(&waiter->ready, &fb->me);
	pthread_mutex_unlock(&waiter->lock);

	// Only one wakeup action can be executed in the same time.
	if (atomic_int64_cas(waiter->atomic, 0, 1) != 0) {
		return;
	}

	// Notify the waiter fiber to handle the ready fibers.
	fbase_event_wakeup(&waiter->fb->base);

	if (atomic_int64_cas(waiter->atomic, 1, 0) != 1) {
		abort();
	}
}
