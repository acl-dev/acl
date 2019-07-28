#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "stdafx.h"
#include <dlfcn.h>
#include <poll.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef int (*close_fn)(int);
typedef int (*socket_fn)(int, int, int);
typedef int (*socketpair_fn)(int, int, int, int sv[2]);
typedef int (*bind_fn)(int, const struct sockaddr *, socklen_t);
typedef int (*listen_fn)(int, int);
typedef int (*accept_fn)(int, struct sockaddr *, socklen_t *);
typedef int (*connect_fn)(int, const struct sockaddr *, socklen_t);

typedef int (*poll_fn)(struct pollfd *, nfds_t, int);
typedef int (*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
typedef int (*gethostbyname_r_fn)(const char *, struct hostent *, char *,
	size_t, struct hostent **, int *);
typedef int (*getaddrinfo_fn)(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res);
typedef void (*freeaddrinfo_fn)(struct addrinfo *res);

typedef int (*epoll_create_fn)(int);
typedef int (*epoll_wait_fn)(int, struct epoll_event *,int, int);
typedef int (*epoll_ctl_fn)(int, int, int, struct epoll_event *);

static close_fn           __sys_close           = NULL;
static socket_fn          __sys_socket          = NULL;
static socketpair_fn      __sys_socketpair      = NULL;
static bind_fn            __sys_bind            = NULL;
static listen_fn          __sys_listen          = NULL;
static accept_fn          __sys_accept          = NULL;
static connect_fn         __sys_connect         = NULL;

static poll_fn            __sys_poll            = NULL;
static poll_fn            __sys_xx_poll         = NULL;
static select_fn          __sys_select          = NULL;
static gethostbyname_r_fn __sys_gethostbyname_r = NULL;
static getaddrinfo_fn     __sys_getaddrinfo     = NULL;
static freeaddrinfo_fn    __sys_freeaddrinfo    = NULL;

static epoll_create_fn    __sys_epoll_create    = NULL;
static epoll_wait_fn      __sys_epoll_wait      = NULL;
static epoll_ctl_fn       __sys_epoll_ctl       = NULL;

void hook_net(void)
{
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__called) {
		(void) acl_pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_close      = (close_fn) dlsym(RTLD_NEXT, "close");
	acl_assert(__sys_close);

	__sys_socket     = (socket_fn) dlsym(RTLD_NEXT, "socket");
	acl_assert(__sys_socket);

	__sys_socketpair = (socketpair_fn) dlsym(RTLD_NEXT, "socketpair");
	acl_assert(__sys_socketpair);

	__sys_bind       = (bind_fn) dlsym(RTLD_NEXT, "bind");
	acl_assert(__sys_bind);

	__sys_listen     = (listen_fn) dlsym(RTLD_NEXT, "listen");
	acl_assert(__sys_listen);

	__sys_accept     = (accept_fn) dlsym(RTLD_NEXT, "accept");
	acl_assert(__sys_accept);

	__sys_connect    = (connect_fn) dlsym(RTLD_NEXT, "connect");
	acl_assert(__sys_connect);

	__sys_poll       = (poll_fn) dlsym(RTLD_NEXT, "poll");
	acl_assert(__sys_poll);

	__sys_xx_poll = (poll_fn) dlsym(RTLD_NEXT, "__poll");

	__sys_select     = (select_fn) dlsym(RTLD_NEXT, "select");
	acl_assert(__sys_select);

	__sys_gethostbyname_r = (gethostbyname_r_fn) dlsym(RTLD_NEXT,
			"gethostbyname_r");
	acl_assert(__sys_gethostbyname_r);

	__sys_getaddrinfo = (getaddrinfo_fn) dlsym(RTLD_NEXT, "getaddrinfo");
	acl_assert(__sys_getaddrinfo);

	__sys_freeaddrinfo = (freeaddrinfo_fn) dlsym(RTLD_NEXT,
			"freeaddrinfo");
	acl_assert(__sys_freeaddrinfo);

	__sys_epoll_create = (epoll_create_fn) dlsym(RTLD_NEXT, "epoll_create");
	acl_assert(__sys_epoll_create);

	__sys_epoll_wait   = (epoll_wait_fn) dlsym(RTLD_NEXT, "epoll_wait");
	acl_assert(__sys_epoll_wait);

	__sys_epoll_ctl    = (epoll_ctl_fn) dlsym(RTLD_NEXT, "epoll_ctl");
	acl_assert(__sys_epoll_ctl);

	(void) acl_pthread_mutex_unlock(&__lock);
}

int socket(int domain, int type, int protocol)
{
	int sockfd;

	if (__sys_socket == NULL)
		hook_net();

	if (__sys_socket == NULL)
		return -1;
	sockfd = __sys_socket(domain, type, protocol);

	if (!acl_var_hook_sys_api)
		return sockfd;

	if (sockfd >= 0)
		acl_non_blocking(sockfd, ACL_NON_BLOCKING);
	else
		fiber_save_errno();
	return sockfd;
}

int socketpair(int domain, int type, int protocol, int sv[2])
{
	int ret;

	if (__sys_socketpair == NULL)
		hook_net();

	if (__sys_socketpair == NULL)
		return -1;
	ret = __sys_socketpair(domain, type, protocol, sv);

	if (!acl_var_hook_sys_api)
		return ret;

	if (ret < 0)
		fiber_save_errno();
	return ret;
}

#define FAST_ACCEPT

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if (__sys_bind == NULL)
		hook_net();

	if (__sys_bind == NULL)
		return -1;
	if (__sys_bind(sockfd, addr, addrlen) == 0)
		return 0;

	if (!acl_var_hook_sys_api)
		return -1;

	fiber_save_errno();
	return -1;
}

int listen(int sockfd, int backlog)
{
	if (__sys_listen == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_listen ? __sys_listen(sockfd, backlog) : -1;

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);
	if (__sys_listen(sockfd, backlog) == 0)
		return 0;

	fiber_save_errno();
	return -1;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	ACL_FIBER *me;
	EVENT *ev;
	int    clifd;

	if (sockfd < 0) {
		acl_msg_error("%s: invalid sockfd %d", __FUNCTION__, sockfd);
		return -1;
	}

	if (__sys_accept == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_accept ? __sys_accept(sockfd, addr, addrlen) : -1;

	me = acl_fiber_running();

#ifdef	FAST_ACCEPT

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);

	ev = fiber_io_event();
	if (ev && event_readable(ev, sockfd))
		event_clear_readable(ev, sockfd);

	clifd = __sys_accept(sockfd, addr, addrlen);
	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
#if EAGAIN == EWOULDBLOCK
	if (errno != EAGAIN)
#else
	if (errno != EAGAIN && errno != EWOULDBLOCK)
#endif
		return -1;

	fiber_wait_read(sockfd);
	if (ev)
		event_clear_readable(ev, sockfd);

	if (acl_fiber_killed(me)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));
		return -1;
	}

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
	return clifd;
#else
	ev = fiber_io_event();
	if (ev && event_readable(ev, sockfd)) {
		event_clear_readable(ev, sockfd);

		clifd = __sys_accept(sockfd, addr, addrlen);
		if (clifd > 0)
			return clifd;

		fiber_save_errno();
		return clifd;
	}

	fiber_wait_read(sockfd);
	if (ev)
		event_clear_readable(ev, sockfd);

	if (acl_fiber_killed(me)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed",
			__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(me));
		return -1;
	}

	clifd = __sys_accept(sockfd, addr, addrlen);

	if (clifd >= 0) {
		acl_non_blocking(clifd, ACL_NON_BLOCKING);
		acl_tcp_nodelay(clifd, 1);
		return clifd;
	}

	fiber_save_errno();
	return clifd;
#endif
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	int err;
	socklen_t len;
	ACL_FIBER *me;

	if (__sys_connect == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_connect ? __sys_connect(sockfd, addr, addrlen) : -1;

	me = acl_fiber_running();

	if (acl_fiber_killed(me)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed, %s",
			__FILE__, __LINE__, __FUNCTION__,
			acl_fiber_id(me), acl_last_serror());
		return -1;
	}

	acl_non_blocking(sockfd, ACL_NON_BLOCKING);

	int ret = __sys_connect(sockfd, addr, addrlen);
	if (ret >= 0) {
		acl_tcp_nodelay(sockfd, 1);
		return ret;
	}

	fiber_save_errno();

	if (errno != EINPROGRESS) {
		if (errno == ECONNREFUSED)
			acl_msg_error("%s(%d), %s: connect ECONNREFUSED",
				__FILE__, __LINE__, __FUNCTION__);
		else if (errno == ECONNRESET)
			acl_msg_error("%s(%d), %s: connect ECONNRESET",
				__FILE__, __LINE__, __FUNCTION__);
		else if (errno == ENETDOWN)
			acl_msg_error("%s(%d), %s: connect ENETDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		else if (errno == ENETUNREACH)
			acl_msg_error("%s(%d), %s: connect ENETUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
		else if (errno == EHOSTDOWN)
			acl_msg_error("%s(%d), %s: connect EHOSTDOWN",
				__FILE__, __LINE__, __FUNCTION__);
		else if (errno == EHOSTUNREACH)
			acl_msg_error("%s(%d), %s: connect EHOSTUNREACH",
				__FILE__, __LINE__, __FUNCTION__);
#ifdef	ACL_LINUX
		/* Linux returns EAGAIN instead of ECONNREFUSED
		 * for unix sockets if listen queue is full -- see nginx
		 */
		else if (errno == EAGAIN)
			acl_msg_error("%s(%d), %s: connect EAGAIN",
				__FILE__, __LINE__, __FUNCTION__);
#endif
		else
			acl_msg_error("%s(%d), %s: connect errno=%d, %s",
				__FILE__, __LINE__, __FUNCTION__, errno,
				acl_last_serror());

		return -1;
	}

	fiber_wait_write(sockfd);

	if (acl_fiber_killed(me)) {
		acl_msg_info("%s(%d), %s: fiber-%u was killed, %s",
			__FILE__, __LINE__, __FUNCTION__,
			acl_fiber_id(me), acl_last_serror());
		return -1;
	}

	len = sizeof(err);
	ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
	if (ret == 0 && err == 0)
	{
		char peer[256];
		len = sizeof(peer);
		if (acl_getpeername(sockfd, peer, len) == 0)
			return 0;

		fiber_save_errno();
		acl_msg_error("%s(%d), %s: getpeername error %s, fd: %d",
			__FILE__, __LINE__, __FUNCTION__,
			acl_last_serror(), sockfd);
		return -1;
	}

	acl_set_error(err);

	acl_msg_error("%s(%d): getsockopt error: %s, ret: %d, err: %d",
		__FUNCTION__, __LINE__, acl_last_serror(), ret, err);

	return -1;
}

/****************************************************************************/

#define SET_TIME(x) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    (x) = ((acl_int64) tv.tv_sec) * 1000 + ((acl_int64) tv.tv_usec)/ 1000; \
} while (0)

#define TO_APPL acl_ring_to_appl

/****************************************************************************/

static void poll_events_del(EVENT *ev, POLL_EVENT *pe)
{
	int i;

	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN) {
			event_del_nodelay(ev, pe->fds[i].fd, EVENT_READABLE);
			ev->events[pe->fds[i].fd].pe = NULL;
		}

		if (pe->fds[i].events & POLLOUT) {
			event_del_nodelay(ev, pe->fds[i].fd, EVENT_WRITABLE);
			ev->events[pe->fds[i].fd].pe = NULL;
		}
	}
}

void poll_fibers_free(void)
{
	EVENT *ev = fiber_io_event();

#ifdef	USE_RING
	ACL_RING *head;

	while ((head = acl_ring_pop_head(&ev->poll_list))) {
		POLL_EVENT *pe = TO_APPL(head, POLL_EVENT, me);
#elif	defined(USE_STACK)
	while (1) {
		POLL_EVENT *pe = acl_stack_pop(ev->poll_list);
		if (pe == NULL)
			break;
#else
	while (1) {
		POLL_EVENT *pe = acl_fifo_pop(ev->poll_list);
		if (pe == NULL)
			break;
#endif
		poll_events_del(ev, pe);
		fiber_free(pe->fiber);
	}
}

static void pollfd_callback(EVENT *ev, int fd, void *ctx, int mask)
{
	FILE_EVENT *fe = &ev->events[fd];
	POLL_EVENT *pe = fe->pe;
	struct pollfd *pfd = (struct pollfd *) ctx;
	int n = 0;

	if (mask & EVENT_READABLE) {
		//if (pfd->events & POLLIN)
		//	event_del(ev, fd, EVENT_READABLE);
		pfd->revents |= POLLIN;
		n = 1;
	}

	if (mask & EVENT_WRITABLE) {
		//if (pfd->events & POLLOUT)
		//	event_del(ev, fd, EVENT_WRITABLE);
		pfd->revents |= POLLOUT;
		n |= (1 << 1);
	}

#ifdef	USE_RING
	assert(acl_ring_size(&ev->poll_list) > 0);
#endif

	if (n > 0) {
		acl_assert(pe);
		pe->nready++;
	}
}

static void event_poll_set(EVENT *ev, POLL_EVENT *pe, int timeout)
{
	int i;

#ifdef	USE_RING
	acl_ring_prepend(&ev->poll_list, &pe->me);
#elif	defined(USE_STACK)
	acl_stack_append(ev->poll_list, pe);
#else
	acl_fifo_push_back(ev->poll_list, pe);
#endif

	pe->nready = 0;

	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN) {
			event_add(ev, pe->fds[i].fd, EVENT_READABLE,
				pollfd_callback, &pe->fds[i]);
			ev->events[pe->fds[i].fd].pe = pe;
		}

		if (pe->fds[i].events & POLLOUT) {
			event_add(ev, pe->fds[i].fd, EVENT_WRITABLE,
				pollfd_callback, &pe->fds[i]);
			ev->events[pe->fds[i].fd].pe = pe;
		}

		pe->fds[i].revents = 0;
	}

	if (timeout >= 0 && (ev->timeout < 0 || timeout < ev->timeout))
		ev->timeout = timeout;
}

static void event_poll_clear(EVENT *ev, POLL_EVENT *pe)
{
	int i;

	acl_ring_detach(&pe->me);

	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN)
			event_del(ev, pe->fds[i].fd, EVENT_READABLE);
		if (pe->fds[i].events & POLLOUT)
			event_del(ev, pe->fds[i].fd, EVENT_WRITABLE);
	}
}

static void poll_callback(EVENT *ev acl_unused, POLL_EVENT *pe)
{
	fiber_io_dec();
	acl_fiber_ready(pe->fiber);
}

extern int __poll(struct pollfd fds[], nfds_t nfds, int timeout);

int __poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
	if (__sys_xx_poll == NULL)
		hook_net();
	if (!acl_var_hook_sys_api)
		return __sys_xx_poll ? __sys_xx_poll(fds, nfds, timeout) : -1;

	return poll(fds, nfds, timeout);
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	POLL_EVENT pe;
	EVENT *ev;
	acl_int64 begin, now;

	if (__sys_poll == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_poll ? __sys_poll(fds, nfds, timeout) : -1;

	ev        = fiber_io_event();
	pe.fds    = fds;
	pe.nfds   = nfds;
	pe.fiber  = acl_fiber_running();
	pe.proc   = poll_callback;
	pe.nready = 0;

	SET_TIME(begin);

	while (1) {
		event_poll_set(ev, &pe, timeout);
		fiber_io_inc();
		acl_fiber_switch();

		if (acl_fiber_killed(pe.fiber)) {
			acl_msg_info("%s(%d), %s: fiber-%u was killed, %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(pe.fiber), acl_last_serror());
			pe.nready = -1;
			break;
		}

#ifdef	USE_RING
		if (acl_ring_size(&ev->poll_list) == 0)
#elif	defined(USE_STACK)
		if (acl_stack_size(ev->poll_list) == 0)
#else
		if (acl_fifo_size(ev->poll_list) == 0)
#endif
			ev->timeout = -1;

		if (pe.nready != 0 || timeout == 0)
			break;

		SET_TIME(now);

		if (timeout > 0 && (now - begin >= timeout))
			break;
	}

	event_poll_clear(ev, &pe);
	return pe.nready;
}

int select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout)
{
	struct pollfd *fds;
	int fd, timo, n, nready = 0;

	if (__sys_select == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_select ? __sys_select
			(nfds, readfds, writefds, exceptfds, timeout) : -1;

	fds = (struct pollfd *) acl_mycalloc(nfds + 1, sizeof(struct pollfd));

	for (fd = 0; fd < nfds; fd++) {
		if (readfds && FD_ISSET(fd, readfds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLIN;
		}

		if (writefds && FD_ISSET(fd, writefds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLOUT;
		}

		if (exceptfds && FD_ISSET(fd, exceptfds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLERR | POLLHUP;
		}
	}

	if (timeout != NULL)
		timo = timeout->tv_sec * 1000 + timeout->tv_usec / 1000;
	else
		timo = -1;

	n = poll(fds, nfds, timo);

	if (readfds)
		FD_ZERO(readfds);
	if (writefds)
		FD_ZERO(writefds);
	if (exceptfds)
		FD_ZERO(exceptfds);

	for (fd = 0; fd < nfds && nready < n; fd++) {
		if (fds[fd].fd < 0 || fds[fd].fd != fd)
			continue;

		if (readfds && (fds[fd].revents & POLLIN)) {
			FD_SET(fd, readfds);
			nready++;
		}

		if (writefds && (fds[fd].revents & POLLOUT)) {
			FD_SET(fd, writefds);
			nready++;
		}

		if (exceptfds && (fds[fd].revents & (POLLERR | POLLHUP))) {
			FD_SET(fd, exceptfds);
			nready++;
		}
	}

	acl_myfree(fds);

	return nready;
}

/****************************************************************************/

static void epoll_events_del(EVENT *ev, EPOLL_EVENT *ee)
{
	size_t i;
	int mask;

	for (i = 0; i < ee->nfds; i++) {
		if (ee->fds[i] == NULL)
			continue;

		mask = ee->fds[i]->mask;
		if (mask & EVENT_READABLE)
			event_del_nodelay(ev, ee->fds[i]->fd, EVENT_READABLE);
		if (mask & EVENT_WRITABLE)
			event_del_nodelay(ev, ee->fds[i]->fd, EVENT_WRITABLE);
		acl_myfree(ee->fds[i]);
		ee->fds[i] = NULL;
	}
}

void epoll_fibers_free(void)
{
	EVENT *ev = fiber_io_event();

#ifdef	USE_RING
	ACL_RING *head;

	while ((head = acl_ring_pop_head(&ev->epoll_list))) {
		EPOLL_EVENT *ee = TO_APPL(head, EPOLL_EVENT, me);
#elif	defined(USE_STACK)
	while (1) {
		EPOLL_EVENT *ee = acl_stack_pop(ev->epoll_list);
		if (ee == NULL)
			break;
#else
	while (1) {
		EPOLL_EVENT *ee = acl_fifo_pop(ev->epoll_list);
		if (ee == NULL)
			break;
#endif
		epoll_events_del(ev, ee);
		fiber_free(ee->fiber);
	}
}

static EPOLL_EVENT *epfd_alloc(void)
{
	EPOLL_EVENT *ee = acl_mymalloc(sizeof(EPOLL_EVENT));
	int  maxfd = acl_open_limit(0);

	if (maxfd <= 0)
		acl_msg_fatal("%s(%d), %s: acl_open_limit error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	++maxfd;
	ee->fds  = (EPOLL_CTX **) acl_mymalloc(maxfd * sizeof(EPOLL_CTX *));
	ee->nfds = maxfd;

	return ee;
}

static ACL_ARRAY *__main_epfds = NULL;
static __thread ACL_ARRAY *__epfds = NULL;

static acl_pthread_key_t  __once_key;
static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void thread_free(void *ctx acl_unused)
{
	size_t j;
	ACL_ITER iter;

	if (__epfds == NULL)
		return;

	if (__epfds == __main_epfds)
		__main_epfds = NULL;

	acl_foreach(iter, __epfds) {
		EPOLL_EVENT *ee = (EPOLL_EVENT *) iter.data;

		for (j = 0; j < ee->nfds; j++) {
			if (ee->fds[j] != NULL)
				acl_myfree(ee->fds[j]);
		}

		if (ee->epfd >= 0 && __sys_close(ee->epfd) < 0)
			fiber_save_errno();

		acl_myfree(ee->fds);
		acl_myfree(ee);
	}

	acl_array_free(__epfds, NULL);
	__epfds = NULL;
}

static void main_thread_free(void)
{
	if (__main_epfds) {
		thread_free(__main_epfds);
		__main_epfds = NULL;
	}
}

static void thread_init(void)
{
	if (acl_pthread_key_create(&__once_key, thread_free) != 0)
		acl_msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
}

static EPOLL_EVENT *epoll_event_create(int epfd)
{ 
	EPOLL_EVENT *ee = NULL;
	size_t i;

	/* using thread specific to store the epoll handles for each thread*/
	if (__epfds == NULL) {
		if (acl_pthread_once(&__once_control, thread_init) != 0)
			acl_msg_fatal("%s(%d), %s: pthread_once error %s",
				__FILE__, __LINE__, __FUNCTION__, acl_last_serror());

		__epfds = acl_array_create(5);
		if ((unsigned long) acl_pthread_self() ==
			acl_main_thread_self())
		{
			__main_epfds = __epfds;
			atexit(main_thread_free);
		} else if (acl_pthread_setspecific(__once_key, __epfds) != 0)
			acl_msg_fatal("acl_pthread_setspecific error!");
	}

	ee = epfd_alloc();
	acl_array_append(__epfds, ee);

	/* duplicate the current thread's epoll fd, so we can assosiate the
	 * connection handles with one epoll fd for the current thread, and
	 * use one epoll fd for each thread to handle all fds
	 */
	ee->epfd = dup(epfd);

	for (i = 0; i < ee->nfds; i++)
		ee->fds[i] = NULL;

	return ee;
}

static EPOLL_EVENT *epoll_event_find(int epfd)
{
	ACL_ITER iter;

	if (__epfds == NULL) {
		acl_msg_error("%s(%d), %s: __epfds NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	acl_foreach(iter, __epfds) {
		EPOLL_EVENT *ee = (EPOLL_EVENT *) iter.data;
		if (ee->epfd == epfd)
			return ee;
	}

	return NULL;
}

int epoll_event_close(int epfd)
{
	ACL_ITER iter;
	EPOLL_EVENT *ee = NULL;
	int pos = -1;
	size_t i;

	if (__epfds == NULL || epfd < 0)
		return -1;

	acl_foreach(iter, __epfds) {
		EPOLL_EVENT *e = (EPOLL_EVENT *) iter.data;
		if (e->epfd == epfd) {
			ee = e;
			pos = iter.i;
			break;
		}
	}

	if (ee == NULL)
		return -1;

	for (i = 0; i < ee->nfds; i++) {
		if (ee->fds[i] != NULL)
			acl_myfree(ee->fds[i]);
	}

	acl_myfree(ee->fds);
	acl_myfree(ee);
	acl_array_delete(__epfds, pos, NULL);

	return __sys_close(epfd);
}

int epoll_create(int size acl_unused)
{
	EPOLL_EVENT *ee;
	EVENT *ev;
	int epfd;

	if (__sys_epoll_create == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_epoll_create ? __sys_epoll_create(size) : -1;

	ev = fiber_io_event();
	if (ev == NULL) {
		acl_msg_error("%s(%d), %s: create_event failed %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}

	/* get the current thread's epoll fd */
	epfd = event_handle(ev);
	if (epfd < 0) {
		acl_msg_error("%s(%d), %s: invalid event_handle %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return epfd;
	}

	ee = epoll_event_create(epfd);
	return ee->epfd;
}

#ifdef EPOLL_CLOEXEC
int epoll_create1(int flags)
{
	int epfd = epoll_create(100);
	if (epfd == -1)
		return -1;
	if (flags & EPOLL_CLOEXEC)
		(void) acl_close_on_exec(epfd, 1);
	return epfd;
}
#endif

static void epfd_callback(EVENT *ev acl_unused, int fd, void *ctx, int mask)
{
	EPOLL_CTX  *epx = (EPOLL_CTX *) ctx;
	EPOLL_EVENT *ee = epx->ee;

	acl_assert(ee);

	for (; ee->nready < ee->maxevents;) {
		int n = 0;

		if (mask & EVENT_READABLE) {
			ee->events[ee->nready].events = EPOLLIN;
			n++;
		}

		if (mask & EVENT_WRITABLE) {
			ee->events[ee->nready].events = EPOLLOUT;
			n++;
		}

		if (n == 0) { /* xxx */
			acl_msg_error("%s(%d), %s: invalid mask: %d",
				__FILE__, __LINE__, __FUNCTION__, mask);
			continue;
		}

		memcpy(&ee->events[ee->nready].data, &ee->fds[fd]->data,
			sizeof(ee->fds[fd]->data));

		ee->nready++;

		fiber_io_dec();
		return;
	}

#if 0
	acl_msg_error("%s(%d), %s: too large nready %d >= %d",
		__FILE__, __LINE__, __FUNCTION__, ee->nready, ee->maxevents);
#endif
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	EPOLL_EVENT *ee;
	EVENT *ev;
	int    mask = 0;

	if (__sys_epoll_ctl == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_epoll_ctl ?
			__sys_epoll_ctl(epfd, op, fd, event) : -1;

	ee = epoll_event_find(epfd);
	if (ee == NULL) {
		acl_msg_error("%s(%d), %s: not exist epfd: %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	ev = fiber_io_event();
	if (ev == NULL) {
		acl_msg_error("%s(%d), %s: EVENT NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	if (event->events & EPOLLIN)
		mask |= EVENT_READABLE;
	if (event->events & EPOLLOUT)
		mask |= EVENT_WRITABLE;

	if (op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD) {
		if (ee->fds[fd] == NULL)
			ee->fds[fd] = (EPOLL_CTX *)
				acl_mymalloc(sizeof(EPOLL_CTX));
		ee->fds[fd]->fd    = fd;
		ee->fds[fd]->op    = op;
		ee->fds[fd]->mask  = mask;
		ee->fds[fd]->rmask = EVENT_NONE;
		ee->fds[fd]->ee    = ee;
		memcpy(&ee->fds[fd]->data, &event->data, sizeof(event->data));

		if (event_add(ev, fd, mask, epfd_callback, ee->fds[fd]) < 0) {
			acl_msg_error("%s(%d), %s: event_add error, fd: %d",
				__FILE__, __LINE__, __FUNCTION__, fd);
			return -1;
		}

		return 0;
	} else if (op == EPOLL_CTL_DEL) {
		event_del(ev, fd, EVENT_READABLE | EVENT_WRITABLE);
		if (ee->fds[fd] != NULL) {
			ee->fds[fd]->fd    = -1;
			ee->fds[fd]->op    = 0;
			ee->fds[fd]->mask  = EVENT_NONE;
			ee->fds[fd]->rmask = EVENT_NONE;
			memset(&ee->fds[fd]->data, 0, sizeof(ee->fds[fd]->data));
		}

		return 0;
	} else {
		acl_msg_error("%s(%d), %s: invalid op %d, fd %d",
			__FILE__, __LINE__, __FUNCTION__, op, fd);
		return -1;
	}
}

static void epoll_callback(EVENT *ev acl_unused, EPOLL_EVENT *ee)
{
	acl_fiber_ready(ee->fiber);
}

static void event_epoll_set(EVENT *ev, EPOLL_EVENT *ee, int timeout)
{
#ifdef	USE_RING
	acl_ring_prepend(&ev->epoll_list, &ee->me);
#elif	defined(USE_STACK)
	acl_stack_append(ev->epoll_list, ee);
#else
	acl_fifo_push_back(ev->epoll_list, ee);
#endif

	ee->nready = 0;

	if (timeout >= 0 && (ev->timeout < 0 || timeout < ev->timeout))
		ev->timeout = timeout;
}

int epoll_wait(int epfd, struct epoll_event *events,
	int maxevents, int timeout)
{
	EVENT *ev;
	EPOLL_EVENT *ee;
	acl_int64 begin, now;

	if (__sys_epoll_wait == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_epoll_wait ?
			__sys_epoll_wait(epfd, events, maxevents, timeout) : -1;

	ev = fiber_io_event();
	if (ev == NULL) {
		acl_msg_error("%s(%d), %s: EVENT NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	ee = epoll_event_find(epfd);
	if (ee == NULL) {
		acl_msg_error("%s(%d), %s: not exist epfd %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	ee->events    = events;
	ee->maxevents = maxevents;
	ee->fiber     = acl_fiber_running();
	ee->proc      = epoll_callback;

	SET_TIME(begin);

	while (1) {
		event_epoll_set(ev, ee, timeout);
		fiber_io_inc();
		acl_fiber_switch();

		ev->timeout = -1;

		if (acl_fiber_killed(ee->fiber)) {
			acl_ring_detach(&ee->me);
			acl_msg_info("%s(%d), %s: fiber-%u was killed",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(ee->fiber));
			break;
		}

		if (ee->nready != 0 || timeout == 0)
			break;

		SET_TIME(now);

		if (timeout > 0 && (now - begin >= timeout))
			break;
	}

	return ee->nready;
}

/****************************************************************************/

static const char *__dns_ip_default = "8.8.8.8";
static char __dns_ip[128] = { 0 };
static int  __dns_port = 53;

void acl_fiber_set_dns(const char* ip, int port)
{
	if (ip == NULL || *ip == 0)
		__dns_ip[0] = 0;
	else
		snprintf(__dns_ip, sizeof(__dns_ip), "%s", ip);

	__dns_port = port > 0 ? port : 53;
}

#define SKIP_WHILE(cond, cp) { while (*cp && (cond)) cp++; }

static void get_dns(char *ip, size_t size)
{
	const char *filepath = "/etc/resolv.conf";
	ACL_VSTREAM *fp;
	char buf[4096], *ptr;
	ACL_ARGV *tokens;
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__dns_ip[0] != 0) {
		ACL_SAFE_STRNCPY(ip, __dns_ip, size);
		(void) acl_pthread_mutex_unlock(&__lock);
		return;
	}

	fp = acl_vstream_fopen(filepath, O_RDONLY, 066, 4096);
	if (fp == NULL) {
		(void) acl_pthread_mutex_unlock(&__lock);
		ACL_SAFE_STRNCPY(ip, __dns_ip_default, size);

		return;
	}

	__dns_ip[0] = 0;

	while (acl_vstream_gets_nonl(fp, buf, sizeof(buf)) != ACL_VSTREAM_EOF) {
		ptr = buf;
		SKIP_WHILE(*ptr == ' ' || *ptr == '\t' || *ptr == '#', ptr);
		if (*ptr == 0)
			continue;

		tokens = acl_argv_split(ptr, " \t");
		if (tokens->argc < 2) {
			acl_argv_free(tokens);
			continue;
		}

		if (strcasecmp(tokens->argv[0], "nameserver") != 0) {
			acl_argv_free(tokens);
			continue;
		}

		if (!acl_is_ip(tokens->argv[1])) {
			acl_argv_free(tokens);
			continue;
		}

		snprintf(__dns_ip, sizeof(__dns_ip), "%s", tokens->argv[1]);
		acl_argv_free(tokens);
		break;
	}

	acl_vstream_close(fp);

	if (__dns_ip[0] == 0) {
		(void) acl_pthread_mutex_unlock(&__lock);
		ACL_SAFE_STRNCPY(ip, __dns_ip_default, size);
		return;
	}

	ACL_SAFE_STRNCPY(ip, __dns_ip, size);
	(void) acl_pthread_mutex_unlock(&__lock);
}

struct hostent *gethostbyname(const char *name)
{
	static __thread struct hostent ret, *result;
#define BUF_LEN	4096
	static __thread char buf[BUF_LEN];

	return gethostbyname_r(name, &ret, buf, BUF_LEN, &result, &h_errno)
		== 0 ? result : NULL;
}

int gethostbyname_r(const char *name, struct hostent *ret,
	char *buf, size_t buflen, struct hostent **result, int *h_errnop)
{
	ACL_RES *ns = NULL;
	ACL_DNS_DB *res = NULL;
	size_t n = 0, len, i = 0;
	ACL_ITER iter;
	char dns_ip[64];

#define	RETURN(x) do { \
	if (res) \
		acl_netdb_free(res); \
	if (ns) \
		acl_res_free(ns); \
	return (x); \
} while (0)

	if (__sys_gethostbyname_r == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_gethostbyname_r ?  __sys_gethostbyname_r
			(name, ret, buf, buflen, result, h_errnop) : -1;

	get_dns(dns_ip, sizeof(dns_ip));

	memset(ret, 0, sizeof(struct hostent));
	memset(buf, 0, buflen);

	ns = acl_res_new(dns_ip, __dns_port);
	res = acl_res_lookup(ns, name);

	if (res == NULL) {
		acl_msg_error("%s(%d), %s: acl_res_lookup NULL, name: %s,"
			" dns_ip: %s, dns_port: %d", __FILE__, __LINE__,
			__FUNCTION__, name, dns_ip, __dns_port);
		if (h_errnop)
			*h_errnop = HOST_NOT_FOUND;
		RETURN (-1);
	}

	len = strlen(name);
	n += len;
	if (n >= buflen) {
		acl_msg_error("%s(%d), %s: n(%d) > buflen(%d)", __FILE__,
			__LINE__, __FUNCTION__, (int) n, (int) buflen);
		if (h_errnop)
			*h_errnop = ERANGE;
		RETURN (-1);
	}
	memcpy(buf, name, len);
	buf[len] = 0;
	ret->h_name = buf;
	buf += len + 1;

#define MAX_COUNT	64
	len = 8 * MAX_COUNT;
	n += len;
	if (n >= buflen) {
		acl_msg_error("%s(%d), %s: n(%d) > buflen(%d)", __FILE__,
			__LINE__, __FUNCTION__, (int) n, (int) buflen);
		if (h_errnop)
			*h_errnop = ERANGE;
		RETURN (-1);
	}
	ret->h_addr_list = (char**) buf;
	buf += len;

	acl_foreach(iter, res) {
		ACL_HOSTNAME *h = (ACL_HOSTNAME*) iter.data;
		struct in_addr addr;

		len = sizeof(struct in_addr);
		n += len;
		if (n > buflen)
			break;

		memset(&addr, 0, sizeof(addr));
		addr.s_addr = inet_addr(h->ip);
		memcpy(buf, &addr, len);

		if (i >= MAX_COUNT)
			break;
		ret->h_addr_list[i++] = buf;
		buf += len;
		ret->h_length += len;
	}

	if (i > 0) {
		*result = ret;
		RETURN (0);
	}

	acl_msg_error("%s(%d), %s: i == 0, n: %d, buflen: %d",
		__FILE__, __LINE__, __FUNCTION__, (int) n, (int) buflen);

	if (h_errnop)
		*h_errnop = ERANGE;

	RETURN (-1);
}

static int get_port(const char *service, int socktype)
{
	const char *filepath = "/etc/services";
	ACL_VSTREAM *fp;
	char buf[4096], *ptr, *sport, *transport;
	ACL_ARGV *tokens;
	int port = 0, type;

	if (service == NULL || *service == 0)
		return 0;

	if (acl_alldig(service))
		return atoi(service);

	fp = acl_vstream_fopen(filepath, O_RDONLY, 0600, 4096);
	if (fp == NULL)
		return 0;

	while (acl_vstream_gets_nonl(fp, buf, sizeof(buf)) != ACL_VSTREAM_EOF) {
		ptr = buf;
		SKIP_WHILE(*ptr == ' ' || *ptr == '\t' || *ptr == '#', ptr);
		if (*ptr == 0)
			continue;

		tokens = acl_argv_split(ptr, " \t");
		if (tokens->argc < 2) {
			acl_argv_free(tokens);
			continue;
		}

		sport = tokens->argv[1];
		transport = strchr(sport, '/');
		if (transport == NULL) {
			acl_argv_free(tokens);
			continue;
		}
		*transport++ = 0;
		if (!acl_alldig(sport)) {
			acl_argv_free(tokens);
			continue;
		}

		if (strcasecmp(transport, "tcp") == 0)
			type = SOCK_STREAM;
		else if (strcasecmp(transport, "udp") == 0)
			type = SOCK_DGRAM;
		else {
			acl_argv_free(tokens);
			continue;
		}

		if (type == socktype && !strcmp(tokens->argv[0], service)) {
			port = atoi(sport);
			acl_argv_free(tokens);
			break;
		}

		acl_argv_free(tokens);
	}

	acl_vstream_close(fp);
	return port;
}

struct SOCK_ADDR {
	union {
#ifdef AF_INET6
		struct sockaddr_in6 in6;
#endif
		struct sockaddr_in in;
		struct sockaddr sa;
	} sa;
};

static struct addrinfo *create_addrinfo(const char *ip, short port,
	int socktype, int flags)
{
	struct addrinfo *res;
	size_t addrlen = sizeof(struct SOCK_ADDR);
	struct SOCK_ADDR *sa;

	if (acl_is_ipv4(ip)) {
		sa = (struct SOCK_ADDR *) acl_mycalloc(1, addrlen);
		sa->sa.in.sin_family      = AF_INET;
		sa->sa.in.sin_addr.s_addr = inet_addr(ip);
		sa->sa.in.sin_port        = htons(port);
	}
#ifdef AF_INET6
	else if (acl_is_ipv6(ip)) {
		sa = (struct SOCK_ADDR *) acl_mycalloc(1, addrlen);
		sa->sa.in6.sin6_family = AF_INET6;
		sa->sa.in6.sin6_port   = htons(port);
		if (inet_pton(AF_INET6, ip, &sa->sa.in6.sin6_addr) <= 0) {
			acl_myfree(sa);
			return NULL;
		}
	}
#endif
	else
		return NULL;

	res = (struct addrinfo *) acl_mycalloc(1, sizeof(struct addrinfo));
	res->ai_family   = sa->sa.sa.sa_family;
	res->ai_socktype = socktype;
	res->ai_flags    = flags;
	res->ai_addrlen  = (socklen_t) addrlen;
	res->ai_addr     = (struct sockaddr *) sa;

	return res;
}

int getaddrinfo(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res)
{
	ACL_RES *ns;
	ACL_DNS_DB *db;
	short port;
	ACL_ITER iter;
	char dns_ip[64];
	int  socktype = hints ? hints->ai_socktype : SOCK_STREAM;

	if (__sys_getaddrinfo == NULL)
		hook_net();

	if (!acl_var_hook_sys_api)
		return __sys_getaddrinfo ?
			__sys_getaddrinfo(node, service, hints, res) : -1;

	port = get_port(service, socktype);

	*res = NULL;

	if (acl_is_ip(node)) {
		struct addrinfo *ai = create_addrinfo(node, port, socktype,
			hints ? hints->ai_flags : 0);
		if (ai) {
			ai->ai_next = *res;
			*res = ai;
			return 0;
		} else
			return EAI_NODATA;
	}

	get_dns(dns_ip, sizeof(dns_ip));

	ns = acl_res_new(dns_ip, __dns_port);
	db = acl_res_lookup(ns, node);
	if (db == NULL) {
		acl_msg_error("%s(%d), %s: acl_res_lookup NULL, node: %s,"
			" dns_ip: %s, dns_port: %d", __FILE__, __LINE__,
			__FUNCTION__, node, dns_ip, __dns_port);
		acl_res_free(ns);
		return EAI_NODATA;
	}

	acl_foreach(iter, db) {
		ACL_HOSTNAME *h = (ACL_HOSTNAME *) iter.data;
		struct addrinfo *ai = create_addrinfo(h->ip, port, socktype,
			hints ? hints->ai_flags : 0);
		if (ai) {
			ai->ai_next = *res;
			*res = ai;
		}
	}

	acl_netdb_free(db);
	acl_res_free(ns);
	return 0;
}

void freeaddrinfo(struct addrinfo *res)
{
	if (__sys_freeaddrinfo == NULL)
		hook_net();

	if (!acl_var_hook_sys_api) {
		if (__sys_freeaddrinfo)
			__sys_freeaddrinfo(res);
		return;
	}

	while (res) {
		struct addrinfo *tmp = res;
		res = res->ai_next;
		acl_myfree(tmp->ai_addr);
		acl_myfree(tmp);
	}
}
