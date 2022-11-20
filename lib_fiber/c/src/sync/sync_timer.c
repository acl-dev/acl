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
};

static SYNC_TIMER *sync_timer_new(void)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) mem_calloc(1, sizeof(SYNC_TIMER));
	socket_t in, out;
	FILE_EVENT *fe;

	pthread_mutex_init(&timer->lock, NULL);
	timer->box = mbox_create(MBOX_T_MPSC);
	timer->waiters = timer_cache_create();

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

static int check_expire(EVENT *ev, SYNC_TIMER *timer)
{
	long long now = event_get_stamp(ev), expire = -1;
	TIMER_CACHE_NODE *node = avl_first(&timer->waiters->tree), *next;
	RING_ITER iter;

	expire = node ? node->expire : -1;

	while (node && node->expire >= 0 && node->expire <= now) {
		ring_foreach(iter, &node->ring) {
			SYNC_OBJ *obj = ring_to_appl(iter.ptr, SYNC_OBJ, me);

			// Try to delete the obj from cond's waiters, 0 will
			// be returned if the obj has been in the cond, or else
			// -1 will return, so we just set the DELAYED flag.
			if (fiber_cond_delete_waiter(obj->cond, obj) == 0) {
				obj->status = SYNC_STATUS_TIMEOUT;
				acl_fiber_ready(obj->fb);
			} else {
				obj->status = SYNC_STATUS_DELAYED;
			}
		}

		next = AVL_NEXT(&timer->waiters->tree, node);

		// Remove all the waiters in the node and remove the node.
		timer_cache_free_node(timer->waiters, node);

		node = next;
	}

	if (node && node->expire > expire) {
		expire = node->expire;
	}

	if (expire > now) {
		return (int) (expire - now);
	}
	// xxx?
	return 100;
}

static void handle_wakeup(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	if (obj->delay < 0) {
		acl_fiber_ready(obj->fb);
	} else if (timer_cache_remove_exist(timer->waiters,
			obj->expire, &obj->me)) {
		acl_fiber_ready(obj->fb);
	} else if (obj->status & SYNC_STATUS_DELAYED) {
		acl_fiber_ready(obj->fb);
	} else {
		msg_error("%s(%d): no obj=%p", __FUNCTION__, __LINE__, obj);
	}
}

static void fiber_waiting(ACL_FIBER *fiber fiber_unused, void *ctx)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) ctx;
	EVENT *ev = fiber_io_event();
	SYNC_OBJ *obj;
	int delay = -1, res;

	while (!timer->stop) {
		SYNC_MSG *msg = mbox_read(timer->box, delay, &res);

		if (msg == NULL) {
			delay = check_expire(ev, timer);
			continue;
		}

		obj = msg->obj;

		//assert(obj->fb->status == FIBER_STATUS_SUSPEND);

		switch (msg->action) {
		case SYNC_ACTION_AWAIT:
			assert (obj->delay >= 0);
			obj->expire = event_get_stamp(ev) + obj->delay;
			timer_cache_add(timer->waiters, obj->expire, &obj->me);
			if (delay == -1 || obj->delay < delay) {
				delay = obj->delay;
			}
			break;
		case SYNC_ACTION_WAKEUP:
			handle_wakeup(timer, obj);
			break;
		default:
			msg_fatal("%s(%d): unkown action=%d",
				__FUNCTION__, __LINE__, msg->action);
			break;
		}

		mem_free(msg);

		delay = check_expire(ev, timer);

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

void sync_timer_await(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	if (obj->delay >= 0) {
		SYNC_MSG *msg = (SYNC_MSG*) mem_malloc(sizeof(SYNC_MSG));
		msg->obj = obj;
		msg->action = SYNC_ACTION_AWAIT;
		mbox_send(timer->box, msg);
	}
}

void sync_timer_wakeup(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	SYNC_MSG *msg = (SYNC_MSG*) mem_malloc(sizeof(SYNC_MSG));
	msg->obj = obj;
	msg->action = SYNC_ACTION_WAKEUP;

	if (var_hook_sys_api) {
		socket_t out = mbox_out(timer->box);
		FILE_EVENT *fe = fiber_file_cache_get(out);

		fe->mask |= EVENT_SYSIO;

		mbox_send(timer->box, msg);
		fiber_file_cache_put(fe);
	} else {
		mbox_send(timer->box, msg);
	}
}
