#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "fiber.h"
#include "common/mbox.h"
#include "sync_timer.h"

struct SYNC_TIMER {
	pthread_mutex_t lock;
	ARRAY *waiters;
	ACL_FIBER *fb;
	MBOX *box;
	int stop;
};

static SYNC_TIMER *sync_timer_new(void)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) mem_calloc(1, sizeof(SYNC_TIMER));

	pthread_mutex_init(&timer->lock, NULL);
	timer->waiters = array_create(100, ARRAY_F_UNORDER);
	timer->box = mbox_create(MBOX_T_MPSC);
	return timer;
}

static void sync_timer_free(SYNC_TIMER *timer)
{
	pthread_mutex_destroy(&timer->lock);
	array_free(timer->waiters, NULL);
	mbox_free(timer->box, NULL);
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

static void fiber_waiting(ACL_FIBER *fiber fiber_unused, void *ctx)
{
	SYNC_TIMER *timer = (SYNC_TIMER*) ctx;
	int delay = -1;

	while (!timer->stop) {
		int res;
		SYNC_OBJ *obj = mbox_read(timer->box, delay, &res);
		if (obj) {
			assert(obj->fb->status == FIBER_STATUS_SUSPEND);
			acl_fiber_ready(obj->fb);
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

void sync_timer_add(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	pthread_mutex_lock(&timer->lock);
	array_append(timer->waiters, obj);
	pthread_mutex_unlock(&timer->lock);
}

void sync_timer_wakeup(SYNC_TIMER *timer, SYNC_OBJ *obj)
{
	pthread_mutex_lock(&timer->lock);
	array_delete_obj(timer->waiters, obj, NULL);
	pthread_mutex_unlock(&timer->lock);

	mbox_send(timer->box, obj);
}
