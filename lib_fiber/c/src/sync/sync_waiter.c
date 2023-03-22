#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"
#include "common/mbox.h"
#include "sync_waiter.h"

struct SYNC_WAITER {
	pthread_mutex_t lock;
	ACL_FIBER *fb;
	MBOX *box;
	int stop;
};

static SYNC_WAITER *sync_waiter_new(void)
{
	SYNC_WAITER *waiter = (SYNC_WAITER*) mem_calloc(1, sizeof(SYNC_WAITER));
	socket_t in, out;
	FILE_EVENT *fe;

	pthread_mutex_init(&waiter->lock, NULL);
	waiter->box = mbox_create(MBOX_T_MPSC);

	out = mbox_out(waiter->box);
	assert(out != INVALID_SOCKET);
	fe = fiber_file_open_write(out);
	assert(fe);
	fe->type |= TYPE_INTERNAL | TYPE_EVENTABLE;

	in = mbox_in(waiter->box);
	assert(in != INVALID_SOCKET);
	if (in != out) {
		fe = fiber_file_open_read(in);
		assert(fe);
		fe->type |= TYPE_INTERNAL | TYPE_EVENTABLE;
	}

	return waiter;
}

static void sync_waiter_free(SYNC_WAITER *waiter)
{
	pthread_mutex_destroy(&waiter->lock);
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

void sync_waiter_wakeup(SYNC_WAITER *waiter, ACL_FIBER *fb)
{
	if (var_hook_sys_api) {
		// When using io_uring, we should call the system API of write
		// to send data, because the fd is shared by multiple threads
		// and which can't use io_uring directly, so we set the mask
		// as EVENT_SYSIO to use system write API.

		socket_t out = mbox_out(waiter->box);

		// If the FILE_EVENT got from fiber_file_get() isn't NULL,
		// there must be one fiber suspended due to write waiting
		// with the waiter->box fd. And the current fiber must
		// wait for the previous fiber to return and release the sem.

		FILE_EVENT *fe = fiber_file_get(out);

		if (fe == NULL) {
			fe = fiber_file_cache_get(out);
			// COW(Copy on write) to avoid unnecessary waste:
			// alloc sem binding the fe if necessary.
			if (fe->mbox_wsem == NULL) {
				fe->mbox_wsem = acl_fiber_sem_create(1);
			}
		} else {
			assert(fe->mbox_wsem);
		}

		// Reduce the sem number and maybe be suspended if sem is 0.
		acl_fiber_sem_wait(fe->mbox_wsem);

		fe->mask |= EVENT_SYSIO;

		// The fe mabye be used again in mbox_send->acl_fiber_write
		// ->fiber_file_open_write->fiber_file_get.
		mbox_send(waiter->box, fb);

		// If no other fiber is suspended by the sem, then release it.
		if (acl_fiber_sem_post(fe->mbox_wsem) == 1) {
			fiber_file_cache_put(fe);
		}
	} else {
		mbox_send(waiter->box, fb);
	}
}
