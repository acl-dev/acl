#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"
#include "common/mbox.h"
#include "sync_waiter.h"

struct SYNC_WAITER {
	pthread_mutex_t lock;
	ARRAY *waiters;
	ACL_FIBER *fb;
	MBOX *box;
	int stop;
};

static SYNC_WAITER *sync_waiter_new(void)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) mem_calloc(1, sizeof(SYNC_WAITER));

	pthread_mutex_init(&waiter->lock, NULL);
	waiter->waiters = array_create(100, ARRAY_F_UNORDER);
	waiter->box = mbox_create(MBOX_T_MPSC);
	return waiter;
}

static void sync_waiter_free(SYNC_WAITER *waiter)
{
	pthread_mutex_destroy(&waiter->lock);
	array_free(waiter->waiters, NULL);
	mbox_free(waiter->box, NULL);
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

static void fiber_waiting(ACL_FIBER *fiber fiber_unused, void *ctx)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) ctx;
	int delay = -1;

	while (!waiter->stop) {
		int res;
		ACL_FIBER *fb = mbox_read(waiter->box, delay, &res);
		if (fb) {
			assert(fb->status == FIBER_STATUS_SUSPEND);
			acl_fiber_ready(fb);
		}
	}
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
	}

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
	pthread_mutex_lock(&waiter->lock);
	array_delete_obj(waiter->waiters, fb, NULL);
	pthread_mutex_unlock(&waiter->lock);

	mbox_send(waiter->box, fb);
}
