#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"
#include "common/mbox.h"
#include "sync_timer.h"

struct SYNC_TIMER {
	pthread_mutex_t lock;
	ACL_FIBER *fb;
	MBOX *box;
	int stop;
	TIMER_CACHE *waiters;
	long tid;
};

static SYNC_TIMER *sync_timer_new(void)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) mem_calloc(1, sizeof(SYNC_TIMER));
	socket_t in, out;
	FILE_EVENT *fe;

	pthread_mutex_init(&timer->lock, NULL);
	timer->box = mbox_create(MBOX_T_MPSC);
	timer->waiters = timer_cache_create();
	timer->tid = thread_self();

	out = mbox_out(timer->box);
	assert(out != INVALID_SOCKET);
	fe = fiber_file_open_write(out);
	assert(fe);
	fe->type |= TYPE_INTERNAL | TYPE_EVENTABLE;

	in = mbox_in(timer->box);
	assert(in != INVALID_SOCKET);
	if (in != out) {
		fe = fiber_file_open_read(in);
		assert(fe);
		fe->type |= TYPE_INTERNAL | TYPE_EVENTABLE;
	}

	return timer;
}

static void sync_timer_free(SYNC_TIMER *timer)
{
	pthread_mutex_destroy(&timer->lock);
	mbox_free(timer->box, NULL);
	timer_cache_free(timer->waiters);
	mem_free(timer);
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;
static pthread_key_t  __timer_key;

static void thread_free(void *ctx)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) ctx;
	sync_timer_free(timer);
}

static void thread_init(void)
{
	if (pthread_key_create(&__timer_key, thread_free) != 0) {
		abort();
	}
}

static void wakeup_waiter(SYNC_TIMER *timer UNUSED, SYNC_OBJ *obj)
{
	// The fiber must has been awakened by the other fiber or thread.

	if (obj->delay < 0) {
		// No timer has been set if delay < 0, 
		acl_fiber_ready(obj->fb);
	} else if (fiber_timer_del(obj->fb) == 1) {
		// Wakeup the waiting fiber before the timer arrives,
		// just remove it from the timer.
		acl_fiber_ready(obj->fb);
	}
	// else: The fiber has been awakened by the timer.
}

static void fiber_waiting(ACL_FIBER *fiber fiber_unused, void *ctx)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) ctx;
	SYNC_OBJ *obj;
	int delay = -1, res;

	while (!timer->stop) {
		SYNC_MSG *msg = mbox_read(timer->box, delay, &res);

		if (msg == NULL) {
			continue;
		}

		obj = msg->obj;

		//assert(obj->fb->status == FIBER_STATUS_SUSPEND);

		switch (msg->action) {
		case SYNC_ACTION_WAKEUP:
			wakeup_waiter(timer, obj);
			break;
		default:
			msg_fatal("%s(%d): unkown action=%d",
				__FUNCTION__, __LINE__, msg->action);
			break;
		}

		// Decrease reference to the obj added in sync_timer_wakeup.
		sync_obj_unrefer(obj);
		mem_free(msg);

		if (timer_cache_size(timer->waiters) == 0) {
			delay = -1;
		}
	}
}

SYNC_TIMER *sync_timer_get(void)
{
	SYNC_TIMER *timer;

	if (pthread_once(&__once_control, thread_init) != 0) {
		abort();
	}

	timer = (SYNC_TIMER*) pthread_getspecific(__timer_key);
	if (timer == NULL) {
		timer = sync_timer_new();
		pthread_setspecific(__timer_key, timer);
		timer->fb = acl_fiber_create(fiber_waiting, timer, 320000);
	}

	return timer;
}

void sync_timer_wakeup(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	if (!var_hook_sys_api) {
		// If the current notifier is a thread, just send message.

		SYNC_MSG *msg = (SYNC_MSG*) mem_malloc(sizeof(SYNC_MSG));
		msg->obj = obj;
		msg->action = SYNC_ACTION_WAKEUP;

		// Add a reference to the obj when it's on the fly to avoid
		// the obj was freed in advance, because the obj will be used
		// in fiber_waiting().
		sync_obj_refer(obj);
		mbox_send(timer->box, msg);
	} else if (thread_self() != timer->tid) {
		// If the current notifier is a fiber of another thread, send
		// message with the temporary FILE_EVENT.

		SYNC_MSG *msg = (SYNC_MSG*) mem_malloc(sizeof(SYNC_MSG));
		msg->obj = obj;
		msg->action = SYNC_ACTION_WAKEUP;
		socket_t out = mbox_out(timer->box);
		FILE_EVENT *fe = fiber_file_cache_get(out);

		fe->mask |= EVENT_SYSIO;

		sync_obj_refer(obj);
		mbox_send(timer->box, msg);
		fiber_file_cache_put(fe);
	} else {
		// If the current notifier is a fiber in the same thread with
		// the one to be awakened, just wakeup it directly.

		wakeup_waiter(timer, obj);
	} 
}
