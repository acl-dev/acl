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

	pthread_mutex_init(&timer->lock, NULL);
	timer->box = mbox_create(MBOX_T_MPSC);
	timer->waiters = timer_cache_create();
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
	long long now = event_get_stamp(ev);
	TIMER_CACHE_NODE *node = avl_first(&ev->poll_list->tree), *next;
	RING_ITER iter;

	while (node && node->expire >= 0 && node->expire <= now) {
		next = AVL_NEXT(&timer->waiters->tree, node);
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

		// Remove all the waiters in the node and remove the node.
		timer_cache_free_node(timer->waiters, node);
		node = next;
	}

	if (node == NULL) {
		return -1;
	}

	if (node->expire > now) {
		return node->expire - now;
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
		msg_error("%s(%d): not exist, obj=%p",
			__FUNCTION__, __LINE__, obj);
	}
}

static void fiber_waiting(ACL_FIBER *fiber fiber_unused, void *ctx)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) ctx;
	EVENT *ev = fiber_io_event();
	int delay = -1;

	while (!timer->stop) {
		int res;
		SYNC_OBJ *obj = mbox_read(timer->box, delay, &res);
		if (obj == NULL) {
			delay = check_expire(ev, timer);
			continue;
		}

		//assert(obj->fb->status == FIBER_STATUS_SUSPEND);

		switch (obj->action) {
		case SYNC_ACTION_AWAIT:
			assert (obj->delay >= 0);
			obj->expire = event_get_stamp(ev) + delay;
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
				__FUNCTION__, __LINE__, obj->action);
			break;
		}

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
		obj->action = SYNC_ACTION_AWAIT;
		mbox_send(timer->box, obj);
	}
}

void sync_timer_wakeup(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	obj->action = SYNC_ACTION_WAKEUP;
	mbox_send(timer->box, obj);
}
