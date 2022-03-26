#include "stdafx.h"

#ifdef	HAS_EPOLL

#include "common.h"
#include "fiber/libfiber.h"
#include "event.h"
#include "fiber.h"
#include "hook.h"

/****************************************************************************/

struct EPOLL_CTX {
	int  fd;
	int  op;
	int  mask;
	int  rmask;
	FILE_EVENT  *fe;
	EPOLL_EVENT *ee;
	epoll_data_t data;
};

/**
 * All EPOLL_EVENT owned by its fiber are assosiate with the same one epoll fd.
 * one epoll fd -|- one EPOLL -|- fiber EPOLL_EVENT
 *                             |- fiber EPOLL_EVENT
 *                             |- ...
 *                             |- fiber EPOLL_EVENT -|- socket EPOLL_CTX
 *                                                   |- socket EPOLL_CTX
 *                                                   |- socket EPOLL_CTX
 *                                                   |- ...
 */
struct EPOLL {
	int         epfd;
	EPOLL_CTX **fds;
	size_t      nfds;

	// Store all EPOLL_EVENT, every fiber should use its own EPOLL_EVENT,
	// Because in some case, one thread maybe have many fibers but it maybe
	// use only one epoll fd to handle IO events, see acl_read_epoll_wait()
	// in lib_acl/src/stdlib/iostuff/acl_read_wait.c.
	HTABLE      *ep_events;
};

/****************************************************************************/

#ifdef SYS_WIN
# define SNPRINTF _snprintf
#else
# define SNPRINTF snprintf
#endif

static void epoll_event_free(EPOLL_EVENT *ee)
{
	mem_free(ee);
}

static void fiber_on_exit(void *ctx)
{
	EPOLL_EVENT *ee = (EPOLL_EVENT*) ctx, *tmp;
	ACL_FIBER *curr = acl_fiber_running();
	char key[32];

	assert(curr);

	// If the epoll in ee has been set NULL in epoll_free(), the EPOLL
	// must have been freed and the associated epoll fd must also have
	// been closed, so we just only free the ee here.
	if (ee->epoll == NULL) {
		epoll_event_free(ee);
		return;
	}

	SNPRINTF(key, sizeof(key), "%u", curr->id);
	tmp = (EPOLL_EVENT *) htable_find(ee->epoll->ep_events, key);

	if (tmp == NULL) {
		msg_fatal("%s(%d), %s: not found ee=%p, curr fiber=%d,"
			" ee fiber=%d", __FILE__, __LINE__, __FUNCTION__,
			ee, acl_fiber_id(curr), acl_fiber_id(ee->fiber));
	}

	assert(tmp == ee);
	htable_delete(ee->epoll->ep_events, key, NULL);
	epoll_event_free(ee);
}

static __thread int __local_key;

static EPOLL_EVENT *epoll_event_alloc(void)
{
	// One EPOLL_EVENT can be owned by one fiber and be stored in the
	// fiber's local store, so the EPOLL_EVENT can be used repeated by
	// its owner fiber, and can be freed when the fiber is exiting.

	EPOLL_EVENT *ee = (EPOLL_EVENT*) acl_fiber_get_specific(__local_key);
	if (ee) {
		return ee;
	}

	ee = mem_calloc(1, sizeof(EPOLL_EVENT));
	acl_fiber_set_specific(&__local_key, ee, fiber_on_exit);

	ring_init(&ee->me);
	ee->fiber = acl_fiber_running();

	return ee;
}

/****************************************************************************/

static ARRAY     *__main_epfds = NULL;
static __thread ARRAY *__epfds = NULL;

static pthread_key_t  __once_key;
static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void epoll_free(EPOLL *ep);

static void thread_free(void *ctx fiber_unused)
{
	ITER iter;

	if (__epfds == NULL) {
		return;
	}

	if (__epfds == __main_epfds) {
		__main_epfds = NULL;
	}

	foreach(iter, __epfds) {
		EPOLL *ep = (EPOLL *) iter.data;

		if (ep->epfd >= 0 && (*sys_close)(ep->epfd) < 0) {
			fiber_save_errno(acl_fiber_last_error());
		}

		epoll_free(ep);
	}

	array_free(__epfds, NULL);
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
	if (pthread_key_create(&__once_key, thread_free) != 0) {
		msg_fatal("%s(%d), %s: pthread_key_create error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
}

static EPOLL *epoll_alloc(int epfd)
{ 
	EPOLL *ep;
	int maxfd = open_limit(0), i;

	if (maxfd <= 0) {
		msg_fatal("%s(%d), %s: open_limit error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}

	/* Using thread local to store the epoll handles for each thread. */
	if (__epfds == NULL) {
		if (pthread_once(&__once_control, thread_init) != 0) {
			msg_fatal("%s(%d), %s: pthread_once error %s",
				__FILE__, __LINE__, __FUNCTION__, last_serror());
		}

		__epfds = array_create(5);
		if (__pthread_self() == main_thread_self()) {
			__main_epfds = __epfds;
			atexit(main_thread_free);
		} else if (pthread_setspecific(__once_key, __epfds) != 0) {
			msg_fatal("pthread_setspecific error!");
		}
	}

	ep = mem_malloc(sizeof(EPOLL));
	array_append(__epfds, ep);

	/* Duplicate the current thread's epoll fd, so we can assosiate the
	 * connection handles with one epoll fd for the current thread, and
	 * use one epoll fd for each thread to handle all fds.
	 */
	ep->epfd = dup(epfd);

	ep->nfds = maxfd;
	ep->fds  = (EPOLL_CTX **) mem_malloc(maxfd * sizeof(EPOLL_CTX *));
	for (i = 0; i < maxfd; i++) {
		ep->fds[i] = NULL;
	}

	ep->ep_events = htable_create(100);
	return ep;
}

static void epoll_free(EPOLL *ep)
{
	ITER iter;
	size_t i;

	// Walk through all EPOLL_EVENT stored in ep_events, and just set their
	// epoll variable to NULL, because they will be freed in fiber_on_exit()
	// when the fiber the EPOLL_EVENT belonging to is exiting.

	foreach(iter, ep->ep_events) {
		EPOLL_EVENT *ee = (EPOLL_EVENT *) iter.data;
		ee->epoll = NULL;
	}

	htable_free(ep->ep_events, NULL);

	for (i = 0; i < ep->nfds; i++) {
		if (ep->fds[i] != NULL) {
			mem_free(ep->fds[i]);
		}
	}

	mem_free(ep->fds);
	mem_free(ep);
}

int epoll_event_close(int epfd)
{
	EVENT *ev;
	int sys_epfd;
	EPOLL *ep = NULL;
	int pos = -1;
	ITER iter;

	if (__epfds == NULL || epfd < 0) {
		return -1;
	}

	foreach(iter, __epfds) {
		EPOLL *tmp = (EPOLL *) iter.data;
		if (tmp->epfd == epfd) {
			ep  = tmp;
			pos = iter.i;
			break;
		}
	}

	if (ep == NULL) {
		return -1;
	}

	ev = fiber_io_event();
	assert(ev);

	sys_epfd = event_handle(ev);
	assert(sys_epfd >= 0);

	// We can't close the epfd same as the internal fiber event's fd.
	// Because we've alloced a new fd as a duplication of internal epfd
	// in epoll_alloc by calling sys API dup(), the epfd here shouldn't
	// be same as the internal epfd.

	if (epfd == sys_epfd) {
		msg_error("%s(%d): can't close the event sys_epfd=%d",
			__FUNCTION__, __LINE__, epfd);
		return -1;
	}

	epoll_free(ep);
	array_delete(__epfds, pos, NULL);

	return (*sys_close)(epfd);
}

static EPOLL_EVENT *epoll_event_find(int epfd, int create)
{
	ACL_FIBER *curr = acl_fiber_running();
	EPOLL *ep = NULL;
	EPOLL_EVENT *ee;
	char key[32];
	ITER iter;

	if (__epfds == NULL) {
		msg_error("%s(%d), %s: __epfds NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	foreach(iter, __epfds) {
		EPOLL *tmp = (EPOLL *) iter.data;
		if (tmp->epfd == epfd) {
			ep = tmp;
			break;
		}
	}

	if (ep == NULL) {
		msg_error("%s(%d, %s: not found epfd=%d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return NULL;
	}

	SNPRINTF(key, sizeof(key), "%u", curr->id);
	ee = (EPOLL_EVENT *) htable_find(ep->ep_events, key);
	if (ee != NULL) {
		ee->epoll = ep;
		return ee;
	}

	if (create) {
		ee = epoll_event_alloc();
		ee->epoll = ep;
		htable_enter(ep->ep_events, key, ee);

		return ee;
	} else {
		return NULL;
	}
}

/****************************************************************************/

int epoll_create(int size fiber_unused)
{
	EVENT *ev;
	EPOLL *ep;
	int    epfd;

	if (sys_epoll_create == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return sys_epoll_create ? (*sys_epoll_create)(size) : -1;
	}

	ev = fiber_io_event();
	assert(ev);

	// Get the current thread's epoll fd.
	epfd = event_handle(ev);
	if (epfd < 0) {
		msg_error("%s(%d), %s: invalid event_handle %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return epfd;
	}

	// The epoll fd will be duplicated in the below function, and the new
	// fd will be returned to the caller.
	ep = epoll_alloc(epfd);
	return ep->epfd;
}

#ifdef EPOLL_CLOEXEC
int epoll_create1(int flags)
{
	int epfd = epoll_create(100);

	if (epfd == -1) {
		return -1;
	}
	if (flags & EPOLL_CLOEXEC) {
		(void) close_on_exec(epfd, 1);
	}
	return epfd;
}
#endif

static void read_callback(EVENT *ev, FILE_EVENT *fe)
{
	EPOLL_CTX   *epx = fe->epx;
	EPOLL_EVENT *ee;
	EPOLL       *ep;

	assert(epx);
	assert(epx->mask & EVENT_READ);

	ee = epx->ee;
	assert(ee);

	ep = ee->epoll;
	assert(ep);

	// If the ready count exceeds the maxevents been set which limits the
	// the buffer space to hold the the ready fds, we just return to let
	// the left ready fds keeped in system buffer, and hope they'll be
	// handled in the next epoll_wait().
	if (ee->nready >= ee->maxevents) {
		return;
	}

	ee->events[ee->nready].events |= EPOLLIN;
	memcpy(&ee->events[ee->nready].data, &ep->fds[epx->fd]->data,
		sizeof(ep->fds[epx->fd]->data));

	if (ee->nready == 0) {
		timer_cache_remove(ev->epoll_list, ee->expire, &ee->me);
		ring_prepend(&ev->epoll_ready, &ee->me);
	}

	if (!(ee->events[ee->nready].events & EPOLLOUT)) {
		ee->nready++;
	}

	SET_READABLE(fe);
}

static void write_callback(EVENT *ev fiber_unused, FILE_EVENT *fe)
{
	EPOLL_CTX   *epx = fe->epx;
	EPOLL_EVENT *ee;
	EPOLL       *ep;

	assert(epx);
	assert(epx->mask & EVENT_WRITE);

	ee = epx->ee;
	assert(ee);

	ep = ee->epoll;
	assert(ep);

	if (ee->nready >= ee->maxevents) {
		return;
	}

	ee->events[ee->nready].events |= EPOLLOUT;
	memcpy(&ee->events[ee->nready].data, &ep->fds[epx->fd]->data,
		sizeof(ep->fds[epx->fd]->data));

	if (ee->nready == 0) {
		timer_cache_remove(ev->epoll_list, ee->expire, &ee->me);
		ring_prepend(&ev->epoll_ready, &ee->me);
	}

	if (!(ee->events[ee->nready].events & EPOLLIN)) {
		ee->nready++;
	}

	SET_WRITABLE(fe);
}

static void epoll_ctl_add(EVENT *ev, EPOLL_EVENT *ee,
	struct epoll_event *event, int fd, int op)
{
	EPOLL *ep = ee->epoll;

	if (ep->fds[fd] == NULL) {
		ep->fds[fd] = (EPOLL_CTX *) mem_malloc(sizeof(EPOLL_CTX));
	}

	ep->fds[fd]->fd      = fd;
	ep->fds[fd]->op      = op;
	ep->fds[fd]->mask    = EVENT_NONE;
	ep->fds[fd]->rmask   = EVENT_NONE;
	ep->fds[fd]->ee      = ee;

	memcpy(&ep->fds[fd]->data, &event->data, sizeof(event->data));

	if (event->events & EPOLLIN) {
		ep->fds[fd]->mask   |= EVENT_READ;
		ep->fds[fd]->fe      = fiber_file_open_read(fd);
		ep->fds[fd]->fe->epx = ep->fds[fd];

		event_add_read(ev, ep->fds[fd]->fe, read_callback);
		SET_READWAIT(ep->fds[fd]->fe);
	}

	if (event->events & EPOLLOUT) {
		ep->fds[fd]->mask   |= EVENT_WRITE;
		ep->fds[fd]->fe      = fiber_file_open_write(fd);
		ep->fds[fd]->fe->epx = ep->fds[fd];

		event_add_write(ev, ep->fds[fd]->fe, write_callback);
		SET_WRITEWAIT(ep->fds[fd]->fe);
	}
}

static void epoll_ctl_del(EVENT *ev, EPOLL_EVENT *ee, int fd)
{
	EPOLL *ep = ee->epoll;

	if (ep->fds[fd]->mask & EVENT_READ) {
		event_del_read(ev, ep->fds[fd]->fe);
		CLR_READWAIT(ep->fds[fd]->fe);
	}

	if (ep->fds[fd]->mask & EVENT_WRITE) {
		event_del_write(ev, ep->fds[fd]->fe);
		CLR_WRITEWAIT(ep->fds[fd]->fe);
	}

	ep->fds[fd]->fd      = -1;
	ep->fds[fd]->op      = 0;
	ep->fds[fd]->mask    = EVENT_NONE;
	ep->fds[fd]->rmask   = EVENT_NONE;
	ep->fds[fd]->fe->epx = NULL;
	ep->fds[fd]->fe      = NULL;
	memset(&ep->fds[fd]->data, 0, sizeof(ep->fds[fd]->data));

	mem_free(ep->fds[fd]);
	ep->fds[fd] = NULL;
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	EVENT *ev;
	EPOLL_EVENT *ee;

	if (sys_epoll_ctl == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return sys_epoll_ctl ?  (*sys_epoll_ctl)(epfd, op, fd, event) : -1;
	}

	ee = epoll_event_find(epfd, 1);
	if (ee == NULL) {
		msg_error("%s(%d), %s: not exist epfd=%d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	assert(ee->epoll);

	ev = fiber_io_event();

	if (op == EPOLL_CTL_ADD || op == EPOLL_CTL_MOD) {
		epoll_ctl_add(ev, ee, event, fd, op);
	} else if (op != EPOLL_CTL_DEL) {
		msg_error("%s(%d), %s: invalid op %d, fd %d",
			__FILE__, __LINE__, __FUNCTION__, op, fd);
		return -1;
	} else if (ee->epoll->fds[fd] != NULL) {
		epoll_ctl_del(ev, ee, fd);
	} else {
		msg_error("%s(%d), %s: invalid fd=%d",
			__FILE__, __LINE__, __FUNCTION__, fd);
		return -1;
	}

	return 0;
}

static void epoll_callback(EVENT *ev fiber_unused, EPOLL_EVENT *ee)
{
	fiber_io_dec();

	if (ee->fiber->status != FIBER_STATUS_READY) {
		acl_fiber_ready(ee->fiber);
	}
}

static void event_epoll_set(EVENT *ev, EPOLL_EVENT *ee, int timeout)
{
	int i;

	for (i = 0; i < ee->maxevents; i++) {
		ee->events[i].events = 0;
	}

	if (timeout >= 0) {
		ee->expire = event_get_stamp(ev) + timeout;
		if (ev->timeout < 0 || timeout < ev->timeout) {
			ev->timeout = timeout;
		}
	} else {
		ee->expire = -1;
	}
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
	EVENT *ev;
	EPOLL_EVENT *ee;
	long long now;
	int old_timeout;

	if (sys_epoll_wait == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return sys_epoll_wait ?  (*sys_epoll_wait)
			(epfd, events, maxevents, timeout) : -1;
	}

	ev = fiber_io_event();
	if (ev == NULL) {
		msg_error("%s(%d), %s: EVENT NULL",
			__FILE__, __LINE__, __FUNCTION__);
		return -1;
	}

	ee = epoll_event_find(epfd, 0);
	if (ee == NULL) {
		msg_error("%s(%d), %s: not exist epfd %d",
			__FILE__, __LINE__, __FUNCTION__, epfd);
		return -1;
	}

	ee->events    = events;
	ee->maxevents = maxevents;
	ee->fiber     = acl_fiber_running();
	ee->proc      = epoll_callback;
	ee->nready    = 0;

	old_timeout = ev->timeout;
	event_epoll_set(ev, ee, timeout);
	ev->waiter++;

	while (1) {
		timer_cache_add(ev->epoll_list, ee->expire, &ee->me);

		fiber_io_inc();

		ee->fiber->status = FIBER_STATUS_EPOLL_WAIT;
		acl_fiber_switch();

		if (ee->nready == 0) {
			timer_cache_remove(ev->epoll_list, ee->expire, &ee->me);
		}

		ev->timeout = old_timeout;

		if (acl_fiber_killed(ee->fiber)) {
			acl_fiber_set_error(ee->fiber->errnum);
			if (ee->nready == 0) {
				ee->nready = -1;
			}

			msg_info("%s(%d), %s: fiber-%u was killed",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(ee->fiber));
			break;
		}

		if (timer_cache_size(ev->epoll_list) == 0) {
			ev->timeout = -1;
		}

		if (ee->nready != 0 || timeout == 0) {
			break;
		}

		now = event_get_stamp(ev);
		if (ee->expire > 0 && now >= ee->expire) {
			acl_fiber_set_error(FIBER_ETIMEDOUT);
			break;
		}
	}

	ev->waiter--;
	return ee->nready;
}

#endif	// end HAS_EPOLL
