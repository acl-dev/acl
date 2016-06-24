#include "stdafx.h"
#include <fcntl.h>
#define __USE_GNU
#include <dlfcn.h>
#include <sys/stat.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef int     (*close_fn)(int);
typedef ssize_t (*read_fn)(int, void *, size_t);
typedef ssize_t (*readv_fn)(int, const struct iovec *, int);
typedef ssize_t (*recv_fn)(int, void *, size_t, int);
typedef ssize_t (*recvfrom_fn)(int, void *, size_t, int,
	struct sockaddr *, socklen_t *);
typedef ssize_t (*recvmsg_fn)(int, struct msghdr *, int);
typedef ssize_t (*write_fn)(int, const void *, size_t);
typedef ssize_t (*writev_fn)(int, const struct iovec *, int);
typedef ssize_t (*send_fn)(int, const void *, size_t, int);
typedef ssize_t (*sendto_fn)(int, const void *, size_t, int,
	const struct sockaddr *, socklen_t);
typedef ssize_t (*sendmsg_fn)(int, const struct msghdr *, int);

static close_fn    __sys_close    = NULL;
static read_fn     __sys_read     = NULL;
static readv_fn    __sys_readv    = NULL;
static recv_fn     __sys_recv     = NULL;
static recvfrom_fn __sys_recvfrom = NULL;
static recvmsg_fn  __sys_recvmsg  = NULL;

static write_fn    __sys_write    = NULL;
static writev_fn   __sys_writev   = NULL;
static send_fn     __sys_send     = NULL;
static sendto_fn   __sys_sendto   = NULL;
static sendmsg_fn  __sys_sendmsg  = NULL;

typedef struct {
	EVENT   *event;
	FIBER  **io_fibers;
	size_t   io_count;
	FIBER   *ev_fiber;
	ACL_RING ev_timer;
	int      nsleeping;
	int      io_stop;
} FIBER_TLS;

static FIBER_TLS *__main_fiber = NULL;
static __thread FIBER_TLS *__thread_fiber = NULL;

static void fiber_io_loop(FIBER *fiber, void *ctx);

#define MAXFD		1024
#define STACK_SIZE	819200
static int __maxfd    = 1024;

void fiber_io_hook(void)
{
	static int __called = 0;

	if (__called)
		return;

	__called++;

	__sys_close    = (close_fn) dlsym(RTLD_NEXT, "close");
	__sys_read     = (read_fn) dlsym(RTLD_NEXT, "read");
	__sys_readv    = (readv_fn) dlsym(RTLD_NEXT, "readv");
	__sys_recv     = (recv_fn) dlsym(RTLD_NEXT, "recv");
	__sys_recvfrom = (recvfrom_fn) dlsym(RTLD_NEXT, "recvfrom");
	__sys_recvmsg  = (recvmsg_fn) dlsym(RTLD_NEXT, "recvmsg");

	__sys_write    = (write_fn) dlsym(RTLD_NEXT, "write");
	__sys_writev   = (writev_fn) dlsym(RTLD_NEXT, "writev");
	__sys_send     = (send_fn) dlsym(RTLD_NEXT, "send");
	__sys_sendto   = (sendto_fn) dlsym(RTLD_NEXT, "sendto");
	__sys_sendmsg  = (sendmsg_fn) dlsym(RTLD_NEXT, "sendmsg");
}

void fiber_io_stop(void)
{
	fiber_io_check();
	__thread_fiber->io_stop = 1;
}

#define RING_TO_FIBER(r) \
	((FIBER *) ((char *) (r) - offsetof(FIBER, me)))

#define FIRST_FIBER(head) \
	(acl_ring_succ(head) != (head) ? RING_TO_FIBER(acl_ring_succ(head)) : 0)

#define SET_TIME(x) {  \
	gettimeofday(&tv, NULL);  \
	(x) = tv.tv_sec * 1000 + tv.tv_usec / 1000; \
}

static acl_pthread_key_t __fiber_key;

static void thread_free(void *ctx)
{
	FIBER_TLS *tf = (FIBER_TLS *) ctx;

	if (__thread_fiber == NULL)
		return;

	if (tf->event)
		event_free(tf->event);
	if (tf->io_fibers)
		acl_myfree(tf->io_fibers);
	acl_myfree(tf);

	if (__main_fiber == __thread_fiber)
		__main_fiber = NULL;
	__thread_fiber = NULL;
}

static void fiber_io_main_free(void)
{
	if (__main_fiber) {
		thread_free(__main_fiber);
		if (__thread_fiber == __main_fiber)
			__thread_fiber = NULL;
		__main_fiber = NULL;
	}
}

static void thread_init(void)
{
	acl_assert(acl_pthread_key_create(&__fiber_key, thread_free) == 0);
}

static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

void fiber_io_check(void)
{
	if (__thread_fiber != NULL)
		return;

	acl_assert(acl_pthread_once(&__once_control, thread_init) == 0);

	__maxfd = acl_open_limit(0);
	if (__maxfd <= 0)
		__maxfd = MAXFD;

	__thread_fiber = (FIBER_TLS *) acl_mymalloc(sizeof(FIBER_TLS));
	__thread_fiber->event = event_create(__maxfd);
	__thread_fiber->io_fibers = (FIBER **)
		acl_mycalloc(__maxfd, sizeof(FIBER *));
	__thread_fiber->ev_fiber = fiber_create(fiber_io_loop,
			__thread_fiber->event, STACK_SIZE);
	__thread_fiber->io_count = 0;
	__thread_fiber->nsleeping = 0;
	__thread_fiber->io_stop = 0;
	acl_ring_init(&__thread_fiber->ev_timer);

	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		__main_fiber = __thread_fiber;
		atexit(fiber_io_main_free);
	} else if (acl_pthread_setspecific(__fiber_key, __thread_fiber) != 0)
		acl_msg_fatal("acl_pthread_setspecific error!");
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

static void fiber_io_loop(FIBER *self acl_unused, void *ctx)
{
	EVENT *ev = (EVENT *) ctx;
	int timer_left;
	FIBER *timer;
	int now, last = 0;
	struct timeval tv;

	fiber_system();

	for (;;) {
		while (fiber_yield() > 0) {}

		timer = FIRST_FIBER(&__thread_fiber->ev_timer);

		if (timer == NULL)
			timer_left = -1;
		else {
			SET_TIME(now);
			last = now;
			if (now >= timer->when)
				timer_left = 0;
			else
				timer_left = timer->when - now;
		}

		/* add 1 just for the deviation of epoll_wait */
		event_process(ev, timer_left > 0 ?
			timer_left + 1 : timer_left);

		if (__thread_fiber->io_stop) {
			if (__thread_fiber->io_count > 0)
				acl_msg_info("---------waiting io: %d-----",
					(int) __thread_fiber->io_count);
			break;
		}

		if (timer == NULL)
			continue;

		SET_TIME(now);

		if (now - last < timer_left)
			continue;

		do {
			acl_ring_detach(&timer->me);

			if (!timer->sys && --__thread_fiber->nsleeping == 0)
				fiber_count_dec();

			fiber_ready(timer);
			timer = FIRST_FIBER(&__thread_fiber->ev_timer);

		} while (timer != NULL && now >= timer->when);
	}
}

unsigned int fiber_delay(unsigned int milliseconds)
{
	acl_int64 when, now;
	struct timeval tv;
	FIBER *fiber;
	ACL_RING_ITER iter;

	fiber_io_check();

	SET_TIME(when);
	when += milliseconds;

	acl_ring_foreach_reverse(iter, &__thread_fiber->ev_timer) {
		fiber = acl_ring_to_appl(iter.ptr, FIBER, me);
		if (when >= fiber->when)
			break;
	}

	fiber = fiber_running();
	fiber->when = when;
	acl_ring_detach(&fiber->me);

	acl_ring_append(iter.ptr, &fiber->me);

	if (!fiber->sys && __thread_fiber->nsleeping++ == 0)
		fiber_count_inc();

	fiber_switch();

	SET_TIME(now);

	if (now < when)
		return 0;
	return (unsigned int) (now - when);
}

static void fiber_timer_callback(FIBER *fiber, void *ctx)
{
	struct timeval tv;
	acl_int64 now, left;

	SET_TIME(now);

	for (;;) {
		left = fiber->when > now ? fiber->when - now : 0;
		if (left == 0)
			break;

		fiber_delay(left);

		SET_TIME(now);
		if (fiber->when <= now)
			break;
	}

	fiber->timer_fn(fiber, ctx);
	fiber_exit(0);
}

FIBER *fiber_create_timer(unsigned int milliseconds,
	void (*fn)(FIBER *, void *), void *ctx)
{
	acl_int64 when;
	struct timeval tv;
	FIBER *fiber;

	fiber_io_check();

	SET_TIME(when);
	when += milliseconds;

	fiber           = fiber_create(fiber_timer_callback, ctx, 4000);
	fiber->when     = when;
	fiber->timer_fn = fn;
	return fiber;
}

void fiber_reset_timer(FIBER *fiber, unsigned int milliseconds)
{
	acl_int64 when;
	struct timeval tv;

	fiber_io_check();

	SET_TIME(when);
	when += milliseconds;
	fiber->when = when;
	fiber->status = FIBER_STATUS_READY;
}

unsigned int fiber_sleep(unsigned int seconds)
{
	return fiber_delay(seconds * 1000) / 1000;
}

static void read_callback(EVENT *ev, int fd, void *ctx acl_unused, int mask)
{
	event_del(ev, fd, mask);
	fiber_ready(__thread_fiber->io_fibers[fd]);

	__thread_fiber->io_count--;
	__thread_fiber->io_fibers[fd] = NULL;
}

void fiber_wait_read(int fd)
{
	fiber_io_check();

	if (event_add(__thread_fiber->event,
		fd, EVENT_READABLE, read_callback, NULL) <= 0)
	{
		acl_msg_info(">>>%s(%d): fd: %d, not sock<<<",
			__FUNCTION__, __LINE__, fd);
		return;
	}

	__thread_fiber->io_fibers[fd] = fiber_running();
	__thread_fiber->io_count++;

	fiber_switch();
}

static void write_callback(EVENT *ev, int fd, void *ctx acl_unused, int mask)
{
	event_del(ev, fd, mask);
	fiber_ready(__thread_fiber->io_fibers[fd]);

	__thread_fiber->io_count--;
	__thread_fiber->io_fibers[fd] = NULL;
}

void fiber_wait_write(int fd)
{
	fiber_io_check();

	if (event_add(__thread_fiber->event, fd,
		EVENT_WRITABLE, write_callback, NULL) <= 0)
	{
		return;
	}

	__thread_fiber->io_fibers[fd] = fiber_running();
	__thread_fiber->io_count++;

	fiber_switch();
}

int close(int fd)
{
	int ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	if (__thread_fiber != NULL)
		event_del(__thread_fiber->event, fd, EVENT_ERROR);

	ret = __sys_close(fd);
	if (ret == 0)
		return ret;

	fiber_save_errno();
	return ret;
}

#define READ_WAIT_FIRST

#ifdef READ_WAIT_FIRST

ssize_t read(int fd, void *buf, size_t count)
{
	ssize_t ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	fiber_wait_read(fd);

	ret = __sys_read(fd, buf, count);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;

	if (fd < 0) {
		acl_msg_error("%s: invalid fd: %d", __FUNCTION__, fd);
		return -1;
	}

	fiber_wait_read(fd);
	ret = __sys_readv(fd, iov, iovcnt);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	ssize_t ret;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	fiber_wait_read(sockfd);
	ret = __sys_recv(sockfd, buf, len, flags);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	ssize_t ret;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	fiber_wait_read(sockfd);
	ret = __sys_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	ssize_t ret;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd: %d", __FUNCTION__, sockfd);
		return -1;
	}

	fiber_wait_read(sockfd);
	ret = __sys_recvmsg(sockfd, msg, flags);
	if (ret > 0)
		return ret;

	fiber_save_errno();
	return ret;
}

#else

ssize_t read(int fd, void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_read(fd, buf, count);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;
		fiber_wait_read(fd);
	}
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
	while (1) {
		ssize_t n = __sys_readv(fd, iov, iovcnt);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(fd);
	}
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	while (1) {
		ssize_t n = __sys_recv(sockfd, buf, len, flags);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(sockfd);
	}
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
	struct sockaddr *src_addr, socklen_t *addrlen)
{
	while (1) {
		ssize_t n = __sys_recvfrom(sockfd, buf, len, flags,
				src_addr, addrlen);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(sockfd);
	}
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
	while (1) {
		ssize_t n = __sys_recvmsg(sockfd, msg, flags);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_read(sockfd);
	}
}

#endif

ssize_t write(int fd, const void *buf, size_t count)
{
	while (1) {
		ssize_t n = __sys_write(fd, buf, count);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(fd);
	}
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
	while (1) {
		ssize_t n = __sys_writev(fd, iov, iovcnt);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(fd);
	}
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
	while (1) {
		ssize_t n = __sys_send(sockfd, buf, len, flags);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(sockfd);
	}
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
	const struct sockaddr *dest_addr, socklen_t addrlen)
{
	while (1) {
		ssize_t n = __sys_sendto(sockfd, buf, len, flags,
				dest_addr, addrlen);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(sockfd);
	}
}

ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
	while (1) {
		ssize_t n = __sys_sendmsg(sockfd, msg, flags);

		if (n >= 0)
			return n;

		fiber_save_errno();

#if EAGAIN == EWOULDBLOCK
		if (errno != EAGAIN)
#else
		if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
			return -1;

		fiber_wait_write(sockfd);
	}
}
