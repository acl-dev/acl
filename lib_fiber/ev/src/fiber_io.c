#include "stdafx.h"
#include <limits.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef struct {
	EVENT       *event;
	size_t       io_count;
	ACL_FIBER   *ev_fiber;
	ACL_RING     ev_timer;
	int          nsleeping;
	int          io_stop;
	FILE_EVENT **events;
} FIBER_TLS;

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;

static void fiber_io_loop(ACL_FIBER *fiber, void *ctx);

#define MAXFD		1024
#define STACK_SIZE	819200
int var_maxfd = 1024;

void acl_fiber_schedule_stop(void)
{
	fiber_io_check();
	__thread_fiber->io_stop = 1;
}

#define RING_TO_FIBER(r) \
	((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me)))

#define FIRST_FIBER(head) \
	(acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

static acl_pthread_key_t __fiber_key;

static void thread_free(void *ctx)
{
	FIBER_TLS *tf = (FIBER_TLS *) ctx;

	if (__thread_fiber == NULL) {
		return;
	}

	if (tf->event) {
		event_free(tf->event);
		tf->event = NULL;
	}

	acl_myfree(tf->events);
	acl_myfree(tf);

	if (__main_fiber == __thread_fiber) {
		__main_fiber = NULL;
	}
	__thread_fiber = NULL;
}

static void fiber_io_main_free(void)
{
	if (__main_fiber) {
		thread_free(__main_fiber);
		if (__thread_fiber == __main_fiber) {
			__thread_fiber = NULL;
		}
		__main_fiber = NULL;
	}
}

static void thread_init(void)
{
	if (acl_pthread_key_create(&__fiber_key, thread_free) != 0) {
		acl_msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	}
}

static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

void fiber_io_check(void)
{
	if (__thread_fiber != NULL) {
		return;
	}

	if (acl_pthread_once(&__once_control, thread_init) != 0) {
		acl_msg_fatal("%s(%d), %s: pthread_once error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	}

	var_maxfd = acl_open_limit(0);
	if (var_maxfd <= 0) {
		var_maxfd = MAXFD;
	}

	__thread_fiber = (FIBER_TLS *) acl_mymalloc(sizeof(FIBER_TLS));
	__thread_fiber->event = event_create(var_maxfd);
	__thread_fiber->ev_fiber = acl_fiber_create(fiber_io_loop,
			__thread_fiber->event, STACK_SIZE);
	__thread_fiber->io_count = 0;
	__thread_fiber->nsleeping = 0;
	__thread_fiber->io_stop = 0;
	acl_ring_init(&__thread_fiber->ev_timer);

	__thread_fiber->events = (FILE_EVENT **)
		acl_mycalloc(var_maxfd, sizeof(FILE_EVENT*));

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_io_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0) {
		acl_msg_fatal("acl_pthread_setspecific error!");
	}
}

void fiber_io_dec(void)
{
	fiber_io_check();
	__thread_fiber->io_count--;
}

void fiber_io_inc(void)
{
	fiber_io_check();
	__thread_fiber->io_count++;
}

EVENT *fiber_io_event(void)
{
	fiber_io_check();
	return __thread_fiber->event;
}

static void fiber_io_loop(ACL_FIBER *self acl_unused, void *ctx)
{
	EVENT *ev = (EVENT *) ctx;
	ACL_FIBER *timer;
	acl_int64 now, last = 0, left;

	fiber_system();

	for (;;) {
		while (acl_fiber_yield() > 0) {}

		timer = FIRST_FIBER(&__thread_fiber->ev_timer);
		if (timer == NULL) {
			left = -1;
		} else {
			SET_TIME(now);
			last = now;
			if (now >= timer->when)
				left = 0;
			else
				left = timer->when - now;
		}

		assert(left < INT_MAX);

		/* add 1 just for the deviation of epoll_wait */
		event_process(ev, left > 0 ? left + 1 : (int) left);

		if (__thread_fiber->io_stop) {
			break;
		}

		if (timer == NULL) {
			continue;
		}

		SET_TIME(now);

		if (now - last < left) {
			continue;
		}

		do {
			acl_ring_detach(&timer->me);

			if (!timer->sys && --__thread_fiber->nsleeping == 0) {
				fiber_count_dec();
			}

			acl_fiber_ready(timer);
			timer = FIRST_FIBER(&__thread_fiber->ev_timer);

		} while (timer != NULL && now >= timer->when);
	}

	if (__thread_fiber->io_count > 0) {
		acl_msg_info("%s(%d), %s: waiting io: %d", __FILE__, __LINE__,
			__FUNCTION__, (int) __thread_fiber->io_count);
	}
}

#define CHECK_MIN

unsigned int acl_fiber_delay(unsigned int milliseconds)
{
	acl_int64 when, now;
	ACL_FIBER *fiber;
	ACL_RING_ITER iter;
	EVENT *ev;
#ifdef	CHECK_MIN
	acl_int64 min = -1;
#endif

	if (!acl_var_hook_sys_api) {
		acl_doze(milliseconds);
		return 0;
	}

	fiber_io_check();

	ev = fiber_io_event();

	SET_TIME(when);
	when += milliseconds;

	acl_ring_foreach_reverse(iter, &__thread_fiber->ev_timer) {
		fiber = acl_ring_to_appl(iter.ptr, ACL_FIBER, me);
		if (when >= fiber->when) {
#ifdef	CHECK_MIN
			acl_int64 n = when - fiber->when;
			if (min == -1 || n < min) {
				min = n;
			}
#endif
			break;
		}
	}

#ifdef	CHECK_MIN
	if ((min >= 0 && min < ev->timeout) || ev->timeout < 0) {
		ev->timeout = (int) min;
	}
#else
	ev->timeout = 10;
#endif

	fiber = acl_fiber_running();
	fiber->when = when;
	acl_ring_detach(&fiber->me);

	acl_ring_append(iter.ptr, &fiber->me);

	if (!fiber->sys && __thread_fiber->nsleeping++ == 0) {
		fiber_count_inc();
	}

	acl_fiber_switch();

	//acl_ring_detach(&fiber->me);

	if (acl_ring_size(&__thread_fiber->ev_timer) == 0) {
		ev->timeout = -1;
	} else {
		ev->timeout = min;
	}

	SET_TIME(now);
	if (now < when)
		return 0;

	return (unsigned int) (now - when);
}

static void fiber_timer_callback(ACL_FIBER *fiber, void *ctx)
{
	acl_int64 now, left;

	SET_TIME(now);

	for (;;) {
		left = fiber->when > now ? fiber->when - now : 0;
		if (left == 0) {
			break;
		}

		acl_fiber_delay(left);

		SET_TIME(now);
		if (fiber->when <= now) {
			break;
		}
	}

	fiber->timer_fn(fiber, ctx);
	fiber_exit(0);
}

ACL_FIBER *acl_fiber_create_timer(unsigned int milliseconds, size_t size,
	void (*fn)(ACL_FIBER *, void *), void *ctx)
{
	acl_int64 when;
	ACL_FIBER *fiber;

	fiber_io_check();

	SET_TIME(when);
	when += milliseconds;

	fiber           = acl_fiber_create(fiber_timer_callback, ctx, size);
	fiber->when     = when;
	fiber->timer_fn = fn;
	return fiber;
}

void acl_fiber_reset_timer(ACL_FIBER *fiber, unsigned int milliseconds)
{
	acl_int64 when;

	fiber_io_check();

	SET_TIME(when);
	when += milliseconds;
	fiber->when = when;
	fiber->status = FIBER_STATUS_READY;
}

unsigned int acl_fiber_sleep(unsigned int seconds)
{
	return acl_fiber_delay(seconds * 1000) / 1000;
}

static void read_callback(EVENT *ev, FILE_EVENT *fe)
{
	event_del_read(ev, fe);
	acl_fiber_ready(fe->fiber);
	__thread_fiber->io_count--;
}

void fiber_wait_read(FILE_EVENT *fe)
{
	fiber_io_check();

	fe->fiber = acl_fiber_running();
	event_add_read(__thread_fiber->event, fe, read_callback);
	__thread_fiber->io_count++;
	acl_fiber_switch();
}

static void write_callback(EVENT *ev, FILE_EVENT *fe)
{
	event_del_write(ev, fe);
	acl_fiber_ready(fe->fiber);
	__thread_fiber->io_count--;
}

void fiber_wait_write(FILE_EVENT *fe)
{
	fiber_io_check();

	fe->fiber = acl_fiber_running();
	event_add_write(__thread_fiber->event, fe, write_callback);
	__thread_fiber->io_count++;
	acl_fiber_switch();
}

FILE_EVENT *fiber_io_get(int fd)
{
	fiber_io_check();
	if (fd < 0 || fd >= var_maxfd) {
		acl_msg_error("%s(%d): invalid fd=%d",
			__FUNCTION__, __LINE__, fd);
		return NULL;
	}

	return __thread_fiber->events[fd];
}

int fiber_io_set(FILE_EVENT *fe)
{
	fiber_io_check();
	if (fe->fd < 0 || fe->fd >= var_maxfd) {
		acl_msg_error("%s(%d): invalid fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	if (__thread_fiber->events[fe->fd] != NULL) {
		acl_msg_error("%s(%d): exist fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	__thread_fiber->events[fe->fd] = fe;
	return 0;
}

int fiber_io_del(FILE_EVENT *fe)
{
	fiber_io_check();
	if (fe->fd < 0 || fe->fd >= var_maxfd) {
		acl_msg_error("%s(%d): invalid fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	if (__thread_fiber->events[fe->fd] != fe) {
		acl_msg_error("%s(%d): invalid fe=%p, fd=%d, origin=%p",
			__FUNCTION__, __LINE__, fe, fe->fd,
			__thread_fiber->events[fe->fd]);
		return -1;
	}

	__thread_fiber->events[fe->fd] = NULL;
	return 0;
}

FILE_EVENT *fiber_file_event(int fd)
{
	FILE_EVENT *fe = fiber_io_get(fd);

	if (fe == NULL) {
		fe = file_event_alloc(fd);
		fiber_io_set(fe);
	}
	return fe;
}

int fiber_io_close(int fd)
{
	FILE_EVENT *fe;

	fiber_io_check();
	if (fd < 0 || fd >= var_maxfd) {
		acl_msg_error("%s(%d): invalid fd=%d",
			__FUNCTION__, __LINE__, fd);
		return -1;
	}

	fe = fiber_io_get(fd);
	if (fe == NULL) {
		return 0;
	}

	fiber_io_del(fe);
	if (fe->mask)
		acl_ring_detach(&fe->me);
	return 1;
}
