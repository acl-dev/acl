#include "stdafx.h"
#include "common.h"

#include "fiber/libfiber.h"
#include "common/gettimeofday.h"
#include "common/array.h"
#include "common/timer_cache.h"
#include "event.h"
#include "hook/hook.h"
#include "hook/io.h"
#include "fiber.h"

typedef struct {
	EVENT       *event;
	ACL_FIBER   *ev_fiber;
	TIMER_CACHE *ev_timer;
	int          io_stop;
#ifdef SYS_WIN
	HTABLE      *events;
#else
	FILE_EVENT **events;
#endif
	ARRAY       *cache;
	int          cache_max;
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

static void free_file(void *arg)
{
	FILE_EVENT *fe = (FILE_EVENT*) arg;
	file_event_unrefer(fe);
}

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

	timer_cache_free(tf->ev_timer);

#ifdef SYS_WIN
	htable_free(tf->events, NULL);
#else
	mem_free(tf->events);
#endif

	array_free(tf->cache, free_file);
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

static void thread_once(void)
{
	if (pthread_key_create(&__fiber_key, thread_free) != 0) {
		msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
}

static void thread_init(void)
{
	var_maxfd = open_limit(0);
	if (var_maxfd <= 0) {
		var_maxfd = MAXFD;
	}

	__thread_fiber = (FIBER_TLS *) mem_malloc(sizeof(FIBER_TLS));
	__thread_fiber->event = event_create(var_maxfd);
	__thread_fiber->ev_fiber  = acl_fiber_create(fiber_io_loop,
			__thread_fiber->event, STACK_SIZE);
	__thread_fiber->io_stop   = 0;
	__thread_fiber->ev_timer  = timer_cache_create();

#ifdef SYS_WIN
	__thread_fiber->events = htable_create(var_maxfd);
#else
	__thread_fiber->events = (FILE_EVENT **)
		mem_calloc(var_maxfd, sizeof(FILE_EVENT*));
#endif

	__thread_fiber->cache     = array_create(100, ARRAY_F_UNORDER);
	__thread_fiber->cache_max = 1000;

	if (thread_self() == main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_io_main_free);
	} else if (pthread_setspecific(__fiber_key, __thread_fiber) != 0) {
		printf("pthread_setspecific error!\r\n");
		abort();
	}
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

// Notice: don't write log here to avoid recursive calling when user call
// acl_fiber_msg_register() to hook the log process.

void fiber_io_check(void)
{
	if (__thread_fiber == NULL) {
		if (pthread_once(&__once_control, thread_once) != 0) {
			printf("%s(%d), %s: pthread_once error %s\r\n",
				__FILE__, __LINE__, __FUNCTION__, last_serror());
			abort();
		}

		thread_init();
	} else if (__thread_fiber->ev_fiber == NULL) {
		__thread_fiber->ev_fiber  = acl_fiber_create(fiber_io_loop,
				__thread_fiber->event, STACK_SIZE);
		__thread_fiber->io_stop   = 0;
	}
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

void fiber_timer_add(ACL_FIBER *fiber, unsigned milliseconds)
{
	EVENT *ev = fiber_io_event();
	long long now = event_get_stamp(ev);
	TIMER_CACHE_NODE *timer;

	fiber->when = now + milliseconds;
	ring_detach(&fiber->me);
	timer_cache_add(__thread_fiber->ev_timer, fiber->when, &fiber->me);

	/* Compute the event waiting interval according the timers' head */
	timer = TIMER_FIRST(__thread_fiber->ev_timer);

	if (timer->expire <= now) {
		/* If the first timer has been expired, we should wakeup it
		 * immediately, so the event waiting interval should be set 0.
		 */
		ev->timeout = 0;
	} else {
		/* Then we use the interval between the first timer and now */
		ev->timeout = (int) (fiber->when - now);
	}
}

int fiber_timer_del(ACL_FIBER *fiber)
{
	fiber_io_check();

	return timer_cache_remove(__thread_fiber->ev_timer,
			fiber->when, &fiber->me);
}

static void wakeup_timers(TIMER_CACHE *timers, long long now)
{
	TIMER_CACHE_NODE *node = TIMER_FIRST(timers), *next;
	ACL_FIBER *fb;
	RING_ITER iter;

	while (node && node->expire <= now) {
		// Add the fiber objects into temp array, because the
		// ACL_FIBER::me will be used in acl_fiber_ready that we
		// shouldn't use it in the walk through the node's ring.
		ring_foreach(iter, &node->ring) {
			fb = ring_to_appl(iter.ptr, ACL_FIBER, me);
			array_append(timers->objs, fb);
		}

		while ((fb = (ACL_FIBER*) array_pop_back(timers->objs))) {
			// Set the flag that the fiber wakeuped for the
			// timer's arriving.
			fb->flag |= FIBER_F_TIMER;
			acl_fiber_ready(fb);
		}

		next = TIMER_NEXT(timers, node);
		timer_cache_free_node(timers, node);
		node = next;
	}
}

static void fiber_io_loop(ACL_FIBER *self fiber_unused, void *ctx)
{
	EVENT *ev = (EVENT *) ctx;
	TIMER_CACHE_NODE *timer;
	long long now, last = 0, left;

	for (;;) {
		while (acl_fiber_yield() > 0) {}

		timer = TIMER_FIRST(__thread_fiber->ev_timer);
		if (timer == NULL) {
			left = -1;
		} else {
			now  = event_get_stamp(__thread_fiber->event);
			last = now;
			if (now >= timer->expire) {
				left = 0;
			} else {
				left = timer->expire - now;
			}
		}

		//assert(left < INT_MAX);

		/* Add 1 just for the deviation of epoll_wait */
		event_process(ev, left > 0 ? (int) left + 1 : (int) left);

		if (__thread_fiber->io_stop) {
			msg_info("%s(%d): io_stop set!", __FUNCTION__, __LINE__);
			break;
		}


		if (timer == NULL) {
			/* Try again before exiting the IO fiber loop, some
			 * other fiber maybe in the ready queue and wants to
			 * add some IO event.
			 */
			while (acl_fiber_yield() > 0) {}

			if (ev->waiter > 0) {
				continue;
			} else if (ring_size(&ev->events) > 0) {
				continue;
			}
			
#if 0
			// Only sleep fiber alive ?
			timer = TIMER_FIRST(__thread_fiber->ev_timer);
			if (timer) {
				continue;
			}
#endif

			msg_info("%s(%d), tid=%lu: waiter=%u, events=%d",
				__FUNCTION__, __LINE__, thread_self(),
				ev->waiter, ring_size(&ev->events));
			break;
		}

		now = event_get_stamp(__thread_fiber->event);
		if (now - last >= left) {
			wakeup_timers(__thread_fiber->ev_timer, now);
		}

		if (timer_cache_size(__thread_fiber->ev_timer) == 0) {
			__thread_fiber->event->timeout = -1;
		}
	}

	msg_info("%s(%d), tid=%lu: IO fiber exit now",
		__FUNCTION__, __LINE__, thread_self());

	// Don't set ev_fiber NULL here, using fiber_io_clear() to set it NULL
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

unsigned int acl_fiber_delay(unsigned int milliseconds)
{
	long long now;
	ACL_FIBER *fiber;
	EVENT *ev;

	if (!var_hook_sys_api) {
		doze(milliseconds);
		return 0;
	}

	fiber = acl_fiber_running();
	fiber_timer_add(fiber, milliseconds);

	fiber->status = FIBER_STATUS_DELAY;
	WAITER_INC(__thread_fiber->event);

	acl_fiber_switch();

	WAITER_DEC(__thread_fiber->event);

	// Clear the flag been set in wakeup_timers.
	fiber->flag &= ~FIBER_F_TIMER;

	ev = fiber_io_event();
	now = event_get_stamp(ev);
	if (now <= fiber->when) {
		return 0;
	}

	return (unsigned int) (now - fiber->when);
}

typedef struct {
	void *ctx;
	void (*timer_fn)(ACL_FIBER *, void *);
} TIMER_CTX;

static void fiber_timer_callback(ACL_FIBER *fiber, void *ctx)
{
	long long now, left;
	TIMER_CTX *tc = (TIMER_CTX*) ctx;

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

	tc->timer_fn(fiber, tc->ctx);
	mem_free(tc);
	fiber_exit(0);
}

ACL_FIBER *acl_fiber_create_timer(unsigned int milliseconds, size_t size,
	void (*fn)(ACL_FIBER *, void *), void *ctx)
{
	long long when;
	ACL_FIBER *fiber;
	TIMER_CTX *tc = (TIMER_CTX*) mem_malloc(sizeof(TIMER_CTX));

	tc->timer_fn = fn;
	tc->ctx      = ctx;

	fiber_io_check();

	when = fiber_io_stamp();
	when += milliseconds;

	fiber       = acl_fiber_create(fiber_timer_callback, tc, size);
	fiber->when = when;
	return fiber;
}

int acl_fiber_reset_timer(ACL_FIBER *fiber, unsigned int milliseconds)
{
	// The previous timer with the fiber must be removed first.
	int ret = fiber_timer_del(fiber);
	if (ret == 0) {
		msg_error("%s(%d): not found fiber=%p, fid=%d",
			__FUNCTION__, __LINE__, fiber, acl_fiber_id(fiber));
		return -1;
	}

	fiber_timer_add(fiber, milliseconds);
	return 0;
}

unsigned int acl_fiber_sleep(unsigned int seconds)
{
	return acl_fiber_delay(seconds * 1000) / 1000;
}

/****************************************************************************/

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
}

/**
 * Set fd in reading status by adding it to the event set if the fd is a
 * valid socket or pipe, or return immediately if the fd is not a valid
 * socket. In event_add_read the fd holding in fe will be checking if it's
 * a socket for the first time.
 */
int fiber_wait_read(FILE_EVENT *fe)
{
	int ret;

	fiber_io_check();

	fe->fiber_r = acl_fiber_running();

	// When return 0 just let it go continue
	ret = event_add_read(__thread_fiber->event, fe, read_callback);
	if (ret <= 0) {
		return ret;
	}

	fe->fiber_r->status = FIBER_STATUS_WAIT_READ;
	SET_READWAIT(fe);

	if (!(fe->type & TYPE_INTERNAL)) {
		WAITER_INC(__thread_fiber->event);
	}

	acl_fiber_switch();

	if (!(fe->type & TYPE_INTERNAL)) {
		WAITER_DEC(__thread_fiber->event);
	}

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
	SET_WRITEWAIT(fe);

	if (!(fe->type & TYPE_INTERNAL)) {
		WAITER_INC(__thread_fiber->event);
	}

	acl_fiber_switch();

	if (!(fe->type & TYPE_INTERNAL)) {
		WAITER_DEC(__thread_fiber->event);
	}

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
	if (fd <= INVALID_SOCKET || fd >= var_maxfd) {
#ifdef	HAS_IO_URING
		if (EVENT_IS_IO_URING(fiber_io_event())) {
			printf("%s(%d): invalid fd=%d\r\n",
				__FUNCTION__, __LINE__, fd);
		} else {
			msg_error("%s(%d): invalid fd=%d",
				__FUNCTION__, __LINE__, fd);
		}
#else
		msg_error("%s(%d): invalid fd=%d", __FUNCTION__, __LINE__, fd);
#endif
		return NULL;
	}

	return __thread_fiber->events[fd];
#endif
}

void fiber_file_set(FILE_EVENT *fe)
{
#ifdef SYS_WIN
	char key[64];

	//_snprintf(key, sizeof(key), "%u", fe->fd);
	_i64toa(fe->fd, key, 10);

	htable_enter(__thread_fiber->events, key, fe);
#else
	if (fe->fd <= INVALID_SOCKET || fe->fd >= (socket_t) var_maxfd) {
		printf("%s(%d): invalid fd=%d\r\n", __FUNCTION__, __LINE__, fe->fd);
		abort();
	}

	if (__thread_fiber->events[fe->fd] != NULL) {
		printf("%s(%d): exist fd=%d, old=%p new=%p\r\n", __FUNCTION__,
			__LINE__, fe->fd, __thread_fiber->events[fe->fd], fe);
		abort();
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
#ifdef	HAS_IO_URING
		if (var_hook_sys_api && EVENT_IS_IO_URING(fiber_io_event())) {
			fe->mask |= EVENT_DIRECT;
		}
#endif
	}

	/* We can't set the fe's type here because it'll effect the DGRAM IO,
	 * so, we'll set the fe's sock type in event.c.
	 */
	// Don't set fiber_r here, which will be set in fiber_wait_read()
	//fe->fiber_r = acl_fiber_running();
	return fe;
}

FILE_EVENT *fiber_file_open_write(socket_t fd)
{
	FILE_EVENT *fe = fiber_file_get(fd);

	if (fe == NULL) {
		fe = file_event_alloc(fd);
		fiber_file_set(fe);
#ifdef	HAS_IO_URING
		if (var_hook_sys_api && EVENT_IS_IO_URING(fiber_io_event())) {
			fe->mask |= EVENT_DIRECT;
		}
#endif
	}

	// Don't set fiber_w here, which will be set in fiber_wait_write()
	//fe->fiber_w = acl_fiber_running();
	return fe;
}

void acl_fiber_set_sysio(socket_t fd)
{
	FILE_EVENT *fe;

	if (fd == INVALID_SOCKET) {
		return;
	}

	fe = fiber_file_get(fd);
	if (fe == NULL) {
		fe = file_event_alloc(fd);
		fiber_file_set(fe);
	}
	fe->mask |= EVENT_SYSIO;
}

static int fiber_file_del(FILE_EVENT *fe, socket_t fd)
{
#ifdef SYS_WIN
	char key[64];

	if (fd == INVALID_SOCKET || fd >= (socket_t) var_maxfd) {
		msg_error("%s(%d): invalid fd=%d", __FUNCTION__, __LINE__, fd);
		return -1;
	}

	//_snprintf(key, sizeof(key), "%u", fe->fd);
	_i64toa(fd, key, 10);

	htable_delete(__thread_fiber->events, key, NULL);
	return 0;
#else
	if (fd == INVALID_SOCKET || fd >= var_maxfd) {
		msg_error("%s(%d): invalid fd=%d", __FUNCTION__, __LINE__, fd);
		return -1;
	}

	if (__thread_fiber->events[fd] != fe) {
		msg_error("%s(%d): invalid fe=%p, fd=%d, origin=%p",
			__FUNCTION__, __LINE__, fe, fd, __thread_fiber->events[fd]);
		return -1;
	}

	__thread_fiber->events[fd] = NULL;
	return 0;
#endif
}

void fiber_file_free(FILE_EVENT *fe)
{
	socket_t fd = fe->fd;

	// We must set fd INVALID_SOCKET to stop any using the old fd,
	// fe will be freed only when the reference of it is 0.
	fe->fd = INVALID_SOCKET;

	if (fiber_file_del(fe, fd) == 0) {
		file_event_unrefer(fe);
	} else {
		// xxx: What happened?
		msg_error("%s(%d): some error happened for fe=%p, fd=%d",
			__FUNCTION__, __LINE__, fe, fd);
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
#ifdef HAS_IO_URING
		if (EVENT_IS_IO_URING(__thread_fiber->event)) {
			file_cancel(__thread_fiber->event, fe, CANCEL_IO_READ);
		} else {
			acl_fiber_kill(fe->fiber_r);
		}
#else
		acl_fiber_kill(fe->fiber_r);
#endif
	}

	if (IS_WRITEWAIT(fe) && fe->fiber_w && fe->fiber_w != curr
		&& fe->fiber_w->status != FIBER_STATUS_EXITING) {
		//&& fe->fiber_w->status >= FIBER_STATUS_WAIT_READ
		//&& fe->fiber_w->status <= FIBER_STATUS_EPOLL_WAIT) {

		CLR_WRITEWAIT(fe);
		SET_CLOSING(fe);
#ifdef HAS_IO_URING
		if (EVENT_IS_IO_URING(__thread_fiber->event)) {
			file_cancel(__thread_fiber->event, fe, CANCEL_IO_WRITE);
		} else {
			acl_fiber_kill(fe->fiber_w);
		}
#else
		acl_fiber_kill(fe->fiber_w);
#endif
	}
}

/****************************************************************************/

FILE_EVENT *fiber_file_cache_get(socket_t fd)
{
	FILE_EVENT *fe;

	fiber_io_check();

	fe = (FILE_EVENT*) array_pop_back(__thread_fiber->cache);
	if (fe == NULL) {
		fe = file_event_alloc(fd);
	} else {
		file_event_init(fe, fd);
	}

#ifdef	HAS_IO_URING
	if (var_hook_sys_api && EVENT_IS_IO_URING(fiber_io_event())) {
		fe->mask |= EVENT_DIRECT;
	}
#endif
	fiber_file_set(fe);
	return fe;
}

void fiber_file_cache_put(FILE_EVENT *fe)
{
	fiber_file_del(fe, fe->fd);
	fe->fd = INVALID_SOCKET;

	if (array_size(__thread_fiber->cache) < __thread_fiber->cache_max) {
		array_push_back(__thread_fiber->cache, fe);
	} else {
		file_event_unrefer(fe);
	}
}

/****************************************************************************/
