#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "stdafx.h"
#include <dlfcn.h>
#include <sys/epoll.h>
#include <pthread.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef int (*close_fn)(int);
typedef int (*epoll_create_fn)(int);
typedef int (*epoll_wait_fn)(int, struct epoll_event *,int, int);
typedef int (*epoll_ctl_fn)(int, int, int, struct epoll_event *);

static close_fn        __sys_close        = NULL;
static epoll_create_fn __sys_epoll_create = NULL;
static epoll_wait_fn   __sys_epoll_wait   = NULL;
static epoll_ctl_fn    __sys_epoll_ctl    = NULL;

static void hook_init(void)
{
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__called) {
		(void) acl_pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_close        = (close_fn) dlsym(RTLD_NEXT, "close");
	acl_assert(__sys_close);

	__sys_epoll_create = (epoll_create_fn) dlsym(RTLD_NEXT, "epoll_create");
	acl_assert(__sys_epoll_create);

	__sys_epoll_wait   = (epoll_wait_fn) dlsym(RTLD_NEXT, "epoll_wait");
	acl_assert(__sys_epoll_wait);

	__sys_epoll_ctl    = (epoll_ctl_fn) dlsym(RTLD_NEXT, "epoll_ctl");
	acl_assert(__sys_epoll_ctl);

	(void) acl_pthread_mutex_unlock(&__lock);
}

/****************************************************************************/

static EPOLL_EVENT *epfd_alloc(void)
{
	EPOLL_EVENT *ee = acl_mycalloc(1, sizeof(EPOLL_EVENT));
	int  maxfd = acl_open_limit(0);

	if (maxfd <= 0) {
		acl_msg_fatal("%s(%d), %s: acl_open_limit error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	}

	++maxfd;
	ee->fds  = (EPOLL_CTX **) acl_mymalloc(maxfd * sizeof(EPOLL_CTX *));
	ee->nfds = maxfd;

	return ee;
}

static ACL_ARRAY     *__main_epfds = NULL;
static __thread ACL_ARRAY *__epfds = NULL;

static acl_pthread_key_t  __once_key;
static acl_pthread_once_t __once_control = ACL_PTHREAD_ONCE_INIT;

static void thread_free(void *ctx acl_unused)
{
	size_t j;
	ACL_ITER iter;

	if (__epfds == NULL) {
		return;
	}

	if (__epfds == __main_epfds) {
		__main_epfds = NULL;
	}

	acl_foreach(iter, __epfds) {
		EPOLL_EVENT *ee = (EPOLL_EVENT *) iter.data;

		for (j = 0; j < ee->nfds; j++) {
			if (ee->fds[j] != NULL) {
				acl_myfree(ee->fds[j]);
			}
		}

		if (ee->epfd >= 0 && __sys_close(ee->epfd) < 0) {
			fiber_save_errno();
		}

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
	if (acl_pthread_key_create(&__once_key, thread_free) != 0) {
		acl_msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
	}
}

static EPOLL_EVENT *epoll_event_create(int epfd)
{ 
	EPOLL_EVENT *ee = NULL;
	size_t i;

	/* using thread specific to store the epoll handles for each thread*/
	if (__epfds == NULL) {
		if (acl_pthread_once(&__once_control, thread_init) != 0) {
			acl_msg_fatal("%s(%d), %s: pthread_once error %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_last_serror());
		}

		__epfds = acl_array_create(5);
		if ((unsigned long) acl_pthread_self() ==
			acl_main_thread_self()) {

			__main_epfds = __epfds;
			atexit(main_thread_free);
		} else if (acl_pthread_setspecific(__once_key, __epfds) != 0) {
			acl_msg_fatal("acl_pthread_setspecific error!");
		}
	}

	ee = epfd_alloc();
	acl_array_append(__epfds, ee);

	/* duplicate the current thread's epoll fd, so we can assosiate the
	 * connection handles with one epoll fd for the current thread, and
	 * use one epoll fd for each thread to handle all fds
	 */
	ee->epfd = dup(epfd);

	for (i = 0; i < ee->nfds; i++) {
		ee->fds[i] = NULL;
	}

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
		if (ee->epfd == epfd) {
			return ee;
		}
	}

	return NULL;
}

int epoll_event_close(int epfd)
{
	ACL_ITER iter;
	EPOLL_EVENT *ee = NULL;
	int pos = -1;
	size_t i;

	if (__epfds == NULL || epfd < 0) {
		return -1;
	}

	acl_foreach(iter, __epfds) {
		EPOLL_EVENT *e = (EPOLL_EVENT *) iter.data;
		if (e->epfd == epfd) {
			ee  = e;
			pos = iter.i;
			break;
		}
	}

	if (ee == NULL) {
		return -1;
	}

	for (i = 0; i < ee->nfds; i++) {
		if (ee->fds[i] != NULL) {
			acl_myfree(ee->fds[i]);
		}
	}

	acl_myfree(ee->fds);
	acl_myfree(ee);
	acl_array_delete(__epfds, pos, NULL);

	return __sys_close(epfd);
}

/****************************************************************************/

int epoll_create(int size acl_unused)
{
	EPOLL_EVENT *ee;
	EVENT *ev;
	int    epfd;

	if (__sys_epoll_create == NULL) {
		hook_init();
	}

	if (!acl_var_hook_sys_api) {
		return __sys_epoll_create ? __sys_epoll_create(size) : -1;
	}

	ev = fiber_io_event();

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

	if (epfd == -1) {
		return -1;
	}
	if (flags & EPOLL_CLOEXEC) {
		(void) acl_close_on_exec(epfd, 1);
	}
	return epfd;
}
#endif

static void read_callback(EVENT *ev acl_unused, FILE_EVENT *fe)
{
	EPOLL_CTX  *epx = fe->epx;
	EPOLL_EVENT *ee = epx->ee;

	acl_assert(ee);
	acl_assert(ee->nready < ee->maxevents);
	acl_assert(epx->mask & EVENT_READ);

	ee->events[ee->nready].events |= EPOLLIN;
	memcpy(&ee->events[ee->nready].data, &ee->fds[epx->fd]->data,
		sizeof(ee->fds[epx->fd]->data));
	if (!(ee->events[ee->nready].events & EPOLLOUT))
		ee->nready++;
}

static void write_callback(EVENT *ev acl_unused, FILE_EVENT *fe)
{
	EPOLL_CTX  *epx = fe->epx;
	EPOLL_EVENT *ee = epx->ee;

	acl_assert(ee);
	acl_assert(ee->nready < ee->maxevents);
	acl_assert(epx->mask & EVENT_WRITE);

	ee->events[ee->nready].events |= EPOLLOUT;
	memcpy(&ee->events[ee->nready].data, &ee->fds[epx->fd]->data,
		sizeof(ee->fds[epx->fd]->data));
	if (!(ee->events[ee->nready].events & EPOLLIN))
		ee->nready++;
}

static void epoll_ctl_add(EVENT *ev, EPOLL_EVENT *ee,
	struct epoll_event *event, int fd, int op)
{
	if (ee->fds[fd] == NULL) {
		ee->fds[fd] = (EPOLL_CTX *)
			acl_mymalloc(sizeof(EPOLL_CTX));
	}

	ee->fds[fd]->fd      = fd;
	ee->fds[fd]->op      = op;
	ee->fds[fd]->mask    = EVENT_NONE;
	ee->fds[fd]->rmask   = EVENT_NONE;
	ee->fds[fd]->ee      = ee;
	ee->fds[fd]->fe      = fiber_file_event(fd);
	ee->fds[fd]->fe->epx = ee->fds[fd];

	memcpy(&ee->fds[fd]->data, &event->data, sizeof(event->data));

	if (event->events & EPOLLIN) {
		ee->fds[fd]->mask |= EVENT_READ;
		event_add_read(ev, ee->fds[fd]->fe, read_callback);
	}
	if (event->events & EPOLLOUT) {
		ee->fds[fd]->mask |= EVENT_WRITE;
		event_add_write(ev, ee->fds[fd]->fe, write_callback);
	}
}

static void epoll_ctl_del(EVENT *ev, EPOLL_EVENT *ee, int fd)
{
	if (ee->fds[fd]->mask & EVENT_READ)
		event_del_read(ev, ee->fds[fd]->fe);
	if (ee->fds[fd]->mask & EVENT_WRITE)
		event_del_write(ev, ee->fds[fd]->fe);

	ee->fds[fd]->fd      = -1;
	ee->fds[fd]->op      = 0;
	ee->fds[fd]->mask    = EVENT_NONE;
	ee->fds[fd]->rmask   = EVENT_NONE;
	ee->fds[fd]->fe->epx = NULL;
	ee->fds[fd]->fe      = NULL;
	memset(&ee->fds[fd]->data, 0, sizeof(ee->fds[fd]->data));

	acl_myfree(ee->fds[fd]);
	ee->fds[fd] = NULL;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	EPOLL_EVENT *ee;
	EVENT *ev;

	if (__sys_epoll_ctl == NULL) {
		hook_init();
	}

	if (!acl_var_hook_sys_api) {
		return __sys_epoll_ctl ?
			__sys_epoll_ctl(epfd, op, fd, event) : -1;
	}

	ee = epoll_event_find(epfd);
	if (ee == NULL) {
		acl_msg_error("%s(%d), %s: not exist epfd=%d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	ev = fiber_io_event();

	if (op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD) {
		epoll_ctl_add(ev, ee, event, fd, op);
	} else if (op != EPOLL_CTL_DEL) {
		acl_msg_error("%s(%d), %s: invalid op %d, fd %d",
			__FILE__, __LINE__, __FUNCTION__, op, fd);
		return -1;
	} else if (ee->fds[fd] != NULL) {
		epoll_ctl_del(ev, ee, fd);
	} else {
		acl_msg_error("%s(%d), %s: invalid fd=%d",
			__FILE__, __LINE__, __FUNCTION__, fd);
		return -1;
	}

	return 0;
}

static void epoll_callback(EVENT *ev acl_unused, EPOLL_EVENT *ee)
{
	fiber_io_dec();
	acl_fiber_ready(ee->fiber);
}

static void event_epoll_set(EVENT *ev, EPOLL_EVENT *ee, int timeout)
{
	int i;

	for (i = 0; i < ee->maxevents; i++) {
		ee->events[i].events = 0;
	}

	if (timeout >= 0 && (ev->timeout < 0 || timeout < ev->timeout)) {
		ev->timeout = timeout;
	}
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
	EVENT *ev;
	EPOLL_EVENT *ee;
	acl_int64 begin, now;

	if (__sys_epoll_wait == NULL) {
		hook_init();
	}

	if (!acl_var_hook_sys_api) {
		return __sys_epoll_wait ?
			__sys_epoll_wait(epfd, events, maxevents, timeout) : -1;
	}

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
	ee->nready    = 0;
	ee->fiber     = acl_fiber_running();
	ee->proc      = epoll_callback;

	event_epoll_set(ev, ee, timeout);
	SET_TIME(begin);

	while (1) {
#ifdef	USE_RING
		acl_ring_prepend(&ev->epoll_list, &ee->me);
#elif	defined(USE_STACK)
		acl_stack_append(ev->epoll_list, ee);
#else
		acl_fifo_push_back(ev->epoll_list, ee);
#endif

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
		if (ee->nready != 0 || timeout == 0) {
			break;
		}
		SET_TIME(now);
		if (timeout > 0 && (now - begin >= timeout)) {
			break;
		}
	}

	ee->events    = NULL;
	ee->maxevents = 0;
	ee->nready    = 0;
	return ee->nready;
}
