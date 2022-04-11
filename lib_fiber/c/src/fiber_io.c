#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "common/gettimeofday.h"
#include "event.h"
#include "fiber.h"

typedef struct {
	EVENT     *event;
	size_t     io_count;
	ACL_FIBER *ev_fiber;
	RING       ev_timer;
	int        nsleeping;
	int        io_stop;
#ifdef SYS_WIN
	HTABLE     *events;
#else
	FILE_EVENT **events;
#endif
} FIBER_TLS;

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;

static void fiber_io_loop(ACL_FIBER *fiber, void *ctx);

#define MAXFD		10240
#define STACK_SIZE	819200

int var_maxfd = MAXFD;

void acl_fiber_schedule_stop(void)
{
	if (__thread_fiber != NULL) {
		fiber_io_check();
		__thread_fiber->io_stop = 1;
	}
}

#define RING_TO_FIBER(r) \
	((ACL_FIBER *) ((char *) (r) - offsetof(ACL_FIBER, me)))

#define FIRST_FIBER(head) \
	(ring_succ(head) != (head) ? RING_TO_FIBER(ring_succ(head)) : 0)

static pthread_key_t __fiber_key;

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

#ifdef SYS_WIN
	htable_free(tf->events, NULL);
#else
	mem_free(tf->events);
#endif

	mem_free(tf);

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
	if (pthread_key_create(&__fiber_key, thread_free) != 0) {
		msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

// Notice: don't write log here to avoid recursive calling when user call
// acl_fiber_msg_register() to hook the log process.
void fiber_io_check(void)
{
	if (__thread_fiber != NULL) {
		if (__thread_fiber->ev_fiber == NULL) {
			__thread_fiber->ev_fiber  = acl_fiber_create(fiber_io_loop,
				__thread_fiber->event, STACK_SIZE);
			__thread_fiber->io_count  = 0;
			__thread_fiber->nsleeping = 0;
			__thread_fiber->io_stop   = 0;
			ring_init(&__thread_fiber->ev_timer);
		}
		return;
	}

	if (pthread_once(&__once_control, thread_init) != 0) {
		printf("%s(%d), %s: pthread_once error %s\r\n",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		abort();
	}

	var_maxfd = open_limit(0);
	if (var_maxfd <= 0) {
		var_maxfd = MAXFD;
	}

	__thread_fiber = (FIBER_TLS *) mem_malloc(sizeof(FIBER_TLS));
	__thread_fiber->event = event_create(var_maxfd);
	__thread_fiber->ev_fiber  = acl_fiber_create(fiber_io_loop,
			__thread_fiber->event, STACK_SIZE);
	__thread_fiber->io_count  = 0;
	__thread_fiber->nsleeping = 0;
	__thread_fiber->io_stop   = 0;
	ring_init(&__thread_fiber->ev_timer);

#ifdef SYS_WIN
	__thread_fiber->events = htable_create(var_maxfd);
#else
	__thread_fiber->events = (FILE_EVENT **)
		mem_calloc(var_maxfd, sizeof(FILE_EVENT*));
#endif

	if (__pthread_self() == main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_io_main_free);
	} else if (pthread_setspecific(__fiber_key, __thread_fiber) != 0) {
		printf("pthread_setspecific error!\r\n");
		abort();
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

static long long fiber_io_stamp(void)
{
	EVENT *ev = fiber_io_event();
	return event_get_stamp(ev);
}

static void fiber_io_loop(ACL_FIBER *self fiber_unused, void *ctx)
{
	EVENT *ev = (EVENT *) ctx;
	ACL_FIBER *timer;
	long long now, last = 0, left;

	fiber_system();

	for (;;) {
		while (acl_fiber_yield() > 0) {}

		timer = FIRST_FIBER(&__thread_fiber->ev_timer);
		if (timer == NULL) {
			left = -1;
		} else {
			now  = event_get_stamp(__thread_fiber->event);
			last = now;
			if (now >= timer->when) {
				left = 0;
			} else {
				left = timer->when - now;
			}
		}

		assert(left < INT_MAX);

		/* add 1 just for the deviation of epoll_wait */
		event_process(ev, left > 0 ? (int) left + 1 : (int) left);

		if (__thread_fiber->io_stop) {
			break;
		}

		if (timer == NULL) {
			/* Try again before exiting the IO fiber loop, some
			 * other fiber maybe in the ready queue and wants to
			 * add some IO event.
			 */
			while (acl_fiber_yield() > 0) {}

			if (ev->fdcount > 0 || ev->waiter > 0) {
				continue;
			} else if (ring_size(&ev->events) > 0) {
				continue;
			}
			msg_info("%s(%d), tid=%lu: fdcount=0, waiter=%u, events=%d",
				__FUNCTION__, __LINE__, __pthread_self(),
				ev->waiter, ring_size(&ev->events));
			break;
		}

		now = event_get_stamp(__thread_fiber->event);
		if (now - last < left) {
			continue;
		}

		do {
			ring_detach(&timer->me);

			if (!timer->sys && --__thread_fiber->nsleeping == 0) {
				fiber_count_dec();
			}

			timer->status = FIBER_STATUS_NONE;
			acl_fiber_ready(timer);
			timer = FIRST_FIBER(&__thread_fiber->ev_timer);
		} while (timer != NULL && now >= timer->when);
	}

	if (__thread_fiber->io_count > 0) {
		msg_info("%s(%d), %s: waiting io: %d", __FILE__, __LINE__,
			__FUNCTION__, (int) __thread_fiber->io_count);
	}

	msg_info("%s(%d), tid=%lu: IO fiber exit now",
		__FUNCTION__, __LINE__, __pthread_self());

	// don't set ev_fiber NULL here, using fiber_io_clear() to set it NULL
	// in acl_fiber_schedule() after scheduling finished.
	// 
	// __thread_fiber->ev_fiber = NULL;
}

void fiber_io_clear(void)
{
	if (__thread_fiber) {
		__thread_fiber->ev_fiber = NULL;
	}
}

#define CHECK_MIN

unsigned int acl_fiber_delay(unsigned int milliseconds)
{
	long long when, now;
	ACL_FIBER *fiber;
	RING_ITER iter;
	EVENT *ev;

	if (!var_hook_sys_api) {
		doze(milliseconds);
		return 0;
	}

	fiber_io_check();

	ev = fiber_io_event();

	now = event_get_stamp(ev);
	when = now + milliseconds;

	/* The timers in the ring were stored from small to large in ascending
	 * order, and we walk through the ring from head to tail until the
	 * current time is less than the timer's stamp, and the new timer will
	 * be prepend to the timer found.
	 */
	ring_foreach(iter, &__thread_fiber->ev_timer) {
		fiber = ring_to_appl(iter.ptr, ACL_FIBER, me);
		if (when < fiber->when) {
			break;
		}
	}

	fiber = acl_fiber_running();
	fiber->when = when;
	ring_detach(&fiber->me);
	ring_prepend(iter.ptr, &fiber->me);

	if (!fiber->sys && __thread_fiber->nsleeping++ == 0) {
		fiber_count_inc();
	}

#ifdef	CHECK_MIN
	/* compute the event waiting interval according the timers' head */
	fiber = FIRST_FIBER(&__thread_fiber->ev_timer);
	if (fiber->when <= now) {
		/* If the first timer has been expired, we should wakeup it
		 * immediately, so the event waiting interval should be set 0.
		 */
		ev->timeout = 0;
	} else {
		/* Then we use the interval between the first timer and now */
		ev->timeout = (int) (fiber->when - now);
	}
#else
	ev->timeout = 10;
#endif

	acl_fiber_switch();

	if (ring_size(&__thread_fiber->ev_timer) == 0) {
		ev->timeout = -1;
	}

	now = event_get_stamp(ev);
	if (now < when) {
		return 0;
	}

	return (unsigned int) (now - when);
}

static void fiber_timer_callback(ACL_FIBER *fiber, void *ctx)
{
	long long now, left;

	now = fiber_io_stamp();

	for (;;) {
		left = fiber->when > now ? fiber->when - now : 0;
		if (left == 0) {
			break;
		}

		acl_fiber_delay((unsigned int) left);

		now = fiber_io_stamp();
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
	long long when;
	ACL_FIBER *fiber;

	fiber_io_check();

	when = fiber_io_stamp();
	when += milliseconds;

	fiber           = acl_fiber_create(fiber_timer_callback, ctx, size);
	fiber->when     = when;
	fiber->timer_fn = fn;
	return fiber;
}

void acl_fiber_reset_timer(ACL_FIBER *fiber, unsigned int milliseconds)
{
	long long when;

	fiber_io_check();

	when = fiber_io_stamp();
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
	CLR_READWAIT(fe);
	event_del_read(ev, fe);

	/* If the reader fiber has been set in ready status when the
	 * other fiber killed the reader fiber, the reader fiber should
	 * not be set in ready queue again.
	 */
	if (fe->fiber_r->status != FIBER_STATUS_READY) {
		acl_fiber_ready(fe->fiber_r);
	}
	__thread_fiber->io_count--;
}

/**
 * set fd in reading status by adding it to the event set if the fd is a
 * valid socket or pipe, or return immediately if the fd is not a valid
 * socket. In event_add_read the fd holding in fe will be checking if it's
 * a socket for the first time.
 */
int fiber_wait_read(FILE_EVENT *fe)
{
	int ret;

	fiber_io_check();

	fe->fiber_r = acl_fiber_running();

	// when return 0 just let it go continue
	ret = event_add_read(__thread_fiber->event, fe, read_callback);
	if (ret <= 0) {
		return ret;
	}

	fe->fiber_r->status = FIBER_STATUS_WAIT_READ;
	__thread_fiber->io_count++;
	SET_READWAIT(fe);
	acl_fiber_switch();
	return ret;
}

static void write_callback(EVENT *ev, FILE_EVENT *fe)
{
	CLR_WRITEWAIT(fe);
	event_del_write(ev, fe);

	/* If the writer fiber has been set in ready status when the
	 * other fiber killed the writer fiber, the writer fiber should
	 * not be set in ready queue again.
	 */
	if (fe->fiber_w->status != FIBER_STATUS_READY) {
		acl_fiber_ready(fe->fiber_w);
	}
	__thread_fiber->io_count--;
}

int fiber_wait_write(FILE_EVENT *fe)
{
	int ret;

	fiber_io_check();

	fe->fiber_w = acl_fiber_running();
	ret = event_add_write(__thread_fiber->event, fe, write_callback);
	if (ret <= 0) {
		return ret;
	}

	fe->fiber_w->status = FIBER_STATUS_WAIT_WRITE;
	__thread_fiber->io_count++;
	SET_WRITEWAIT(fe);
	acl_fiber_switch();

	return ret;
}

/****************************************************************************/

FILE_EVENT *fiber_file_get(socket_t fd)
{
#ifdef SYS_WIN
	char key[64];

	fiber_io_check();
	//_snprintf(key, sizeof(key), "%u", fd);
	_i64toa(fd, key, 10); // key's space large enougth

	return (FILE_EVENT *) htable_find(__thread_fiber->events, key);
#else
	fiber_io_check();
	if (fd == INVALID_SOCKET || fd >= var_maxfd) {
		msg_error("%s(%d): invalid fd=%d", __FUNCTION__, __LINE__, fd);
		return NULL;
	}

	return __thread_fiber->events[fd];
#endif
}

static void fiber_file_set(FILE_EVENT *fe)
{
#ifdef SYS_WIN
	char key[64];

	//_snprintf(key, sizeof(key), "%u", fe->fd);
	_i64toa(fe->fd, key, 10);

	htable_enter(__thread_fiber->events, key, fe);
#else
	if (fe->fd == INVALID_SOCKET || fe->fd >= (socket_t) var_maxfd) {
		msg_fatal("%s(%d): invalid fd=%d", __FUNCTION__, __LINE__, fe->fd);
	}

	if (__thread_fiber->events[fe->fd] != NULL) {
		msg_fatal("%s(%d): exist fd=%d", __FUNCTION__, __LINE__, fe->fd);
	}

	__thread_fiber->events[fe->fd] = fe;
#endif
}

FILE_EVENT *fiber_file_open_read(socket_t fd)
{
	FILE_EVENT *fe = fiber_file_get(fd);

	if (fe == NULL) {
		fe = file_event_alloc(fd);
		fiber_file_set(fe);
	}

	/* we can't set the fe's type here because it'll effect the DGRAM IO,
	 * so, we'll set the fe's sock type in event.c.
	 */
	fe->fiber_r = acl_fiber_running();
	return fe;
}

FILE_EVENT *fiber_file_open_write(socket_t fd)
{
	FILE_EVENT *fe = fiber_file_get(fd);

	if (fe == NULL) {
		fe = file_event_alloc(fd);
		fiber_file_set(fe);
	}

	fe->fiber_w = acl_fiber_running();
	return fe;
}

static int fiber_file_del(FILE_EVENT *fe)
{
#ifdef SYS_WIN
	char key[64];

	if (fe->fd == INVALID_SOCKET || fe->fd >= (socket_t) var_maxfd) {
		msg_error("%s(%d): invalid fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	//_snprintf(key, sizeof(key), "%u", fe->fd);
	_i64toa(fe->fd, key, 10);

	htable_delete(__thread_fiber->events, key, NULL);
	return 0;
#else
	if (fe->fd == INVALID_SOCKET || fe->fd >= var_maxfd) {
		msg_error("%s(%d): invalid fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	if (__thread_fiber->events[fe->fd] != fe) {
		msg_error("%s(%d): invalid fe=%p, fd=%d, origin=%p",
			__FUNCTION__, __LINE__, fe, fe->fd,
			__thread_fiber->events[fe->fd]);
		return -1;
	}

	__thread_fiber->events[fe->fd] = NULL;
	return 0;
#endif
}

void fiber_file_free(FILE_EVENT *fe)
{
	if (fiber_file_del(fe) == 0) {
		file_event_free(fe);
	} else {
		// xxx: What happened?
		msg_error("Some error happened for fe=%p, fd=%d", fe, fe->fd);
	}
}

void fiber_file_close(FILE_EVENT *fe)
{
	ACL_FIBER *curr;

	assert(fe);
	fiber_io_check();

	// At first, we should remove the IO event for the fd.
	if (!IS_CLOSING(fe)) {
		EVENT *event;

		event = __thread_fiber->event;
		event_close(event, fe);
	}

	curr = acl_fiber_running();

	if (IS_READWAIT(fe) && fe->fiber_r && fe->fiber_r != curr
		&& fe->fiber_r->status != FIBER_STATUS_EXITING) {
		//&& fe->fiber_r->status >= FIBER_STATUS_WAIT_READ
		//&& fe->fiber_r->status <= FIBER_STATUS_EPOLL_WAIT) {

		// The current fiber is closing the other fiber's fd, and the
		// other fiber hoding the fd is blocked by waiting for the
		// fd to be ready, so we just notify the blocked fiber to
		// wakeup from read waiting status.

		SET_CLOSING(fe);
		CLR_READWAIT(fe);
		acl_fiber_kill(fe->fiber_r);
	} else if (IS_WRITEWAIT(fe) && fe->fiber_w && fe->fiber_w != curr
		&& fe->fiber_w->status != FIBER_STATUS_EXITING) {
		//&& fe->fiber_w->status >= FIBER_STATUS_WAIT_READ
		//&& fe->fiber_w->status <= FIBER_STATUS_EPOLL_WAIT) {

		CLR_WRITEWAIT(fe);
		SET_CLOSING(fe);
		acl_fiber_kill(fe->fiber_w);
	}
}
