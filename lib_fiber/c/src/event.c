#include "stdafx.h"
#include "common.h"

#include "event/event_epoll.h"
#include "event/event_kqueue.h"
#include "event/event_select.h"
#include "event/event_poll.h"
#include "event/event_wmsg.h"
#include "event/event_iocp.h"
#include "event.h"

static __thread int __event_mode = FIBER_EVENT_KERNEL;

void event_set(int event_mode)
{
	switch (__event_mode) {
	case FIBER_EVENT_KERNEL:
	case FIBER_EVENT_POLL:
	case FIBER_EVENT_SELECT:
		__event_mode = event_mode;
		break;
	default:
		break;
	}
}

EVENT *event_create(int size)
{
	EVENT *ev = NULL;

	switch (__event_mode) {
	case FIBER_EVENT_POLL:
#ifdef	HAS_POLL
		ev = event_poll_create(size);
#else
		msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	case FIBER_EVENT_SELECT:
		ev = event_select_create(size);
		break;
	case FIBER_EVENT_WMSG:
#ifdef	HAS_WMSG
		ev = event_wmsg_create(size);
#else
		msg_fatal("%s(%d): not support!", __FUNCTION__, __LINE__);
#endif
		break;
	default:
#if	defined(HAS_EPOLL)
		ev = event_epoll_create(size);
#elif	defined(HAS_KQUEUE)
		ev = event_kqueue_create(size);
#elif	defined(HAS_IOCP)
		ev = event_iocp_create(size);
#else
#error	"unknown OS"
#endif
		break;
	}

	assert(ev);
	ring_init(&ev->events);
	ev->timeout = -1;
	ev->setsize = size;
	ev->fdcount = 0;
	ev->maxfd   = -1;
	ev->waiter  = 0;

	SET_TIME(ev->stamp);  // init the event's stamp when create each event
#ifdef HAS_POLL
	ev->poll_list = timer_cache_create();
	ring_init(&ev->poll_ready);
#endif

#ifdef HAS_EPOLL
	ev->epoll_list = timer_cache_create();
	ring_init(&ev->epoll_ready);
#endif
	return ev;
}

const char *event_name(EVENT *ev)
{
	return ev->name();
}

acl_handle_t event_handle(EVENT *ev)
{
	return ev->handle(ev);
}

void event_free(EVENT *ev)
{
	timer_cache_free(ev->poll_list);
#ifdef	HAS_EPOLL
	timer_cache_free(ev->epoll_list);
#endif

	ev->free(ev);
}

long long event_set_stamp(EVENT *ev)
{
	SET_TIME(ev->stamp);  // decrease the SET_TIME's calling count.
	return ev->stamp;
}

long long event_get_stamp(EVENT *ev)
{
	return ev->stamp;
}

#ifdef SYS_WIN
int event_checkfd(EVENT *ev, FILE_EVENT *fe)
{
	if (getsockfamily(fe->fd) >= 0) {
		return 1;
	}
	if (ev->checkfd(ev, fe) == 0) {
		fe->type = TYPE_SPIPE;
		return 1;
	} else {
		fe->type = TYPE_FILE;
		return 0;
	}
}
#else
int event_checkfd(EVENT *ev, FILE_EVENT *fe)
{
#if 0
	struct stat s;

	if (fstat(fe->fd, &s) < 0) {
		msg_info("%s(%d), %s: fd: %d fstat error %s", __FILE__,
			__LINE__, __FUNCTION__, fe->fd, last_serror());
		return -1;
	}

	if (S_ISSOCK(s.st_mode)) {
		return 1;
	}
	if (S_ISFIFO(s.st_mode)) {
		return 1;
	}

	if (S_ISCHR(s.st_mode)) {
		return 1;
	}
	if (isatty(fe->fd)) {
		return 1;
	}

	return ev->checkfd(ev, fe);
#else
	(void) ev;
	/* If we cannot seek, it must be a pipe, socket or fifo, else it
	 * should be a file.
	 */
	if (lseek(fe->fd, (off_t) 0, SEEK_SET) == -1) {
		switch (errno) {
		case ESPIPE:
			fe->type = TYPE_SPIPE;
			acl_fiber_set_error(0);
			return 1;
		case EBADF:
			fe->type = TYPE_BADFD;
			msg_error("%s(%d): badfd=%d, fe=%p",
				__FUNCTION__, __LINE__, fe->fd, fe);
			return -1;
		default:
			fe->type = TYPE_FILE;
			acl_fiber_set_error(0);
			return 0;
		}
	} else {
		fe->type = TYPE_FILE;
		acl_fiber_set_error(0);
		return 0;
	}
#endif
}
#endif

#if 0
static int check_read_wait(EVENT *ev, FILE_EVENT *fe)
{
	if (ev->add_read(ev, fe) == -1) {
		fe->type = TYPE_NOSOCK;
		return -1;
	}

	if (ev->del_read(ev, fe) == -1) {
		fe->type = TYPE_NOSOCK;
		msg_error("%s(%d): del_read failed, fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	fe->type = TYPE_SOCK;
	return 0;
}

static int check_write_wait(EVENT *ev, FILE_EVENT *fe)
{
	if (ev->add_write(ev, fe) == -1) {
		fe->type = TYPE_NOSOCK;
		return -1;
	}

	if (ev->del_write(ev, fe) == -1) {
		fe->type = TYPE_NOSOCK;
		msg_error("%s(%d): del_write failed, fd=%d",
			__FUNCTION__, __LINE__, fe->fd);
		return -1;
	}

	fe->type = TYPE_SOCK;
	return 0;
}
#endif

int event_add_read(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	if (fe->type == TYPE_NONE) {
		int ret = event_checkfd(ev, fe);
		if (ret <= 0) {
			return ret;
		}
	}

	// if the fd's type has been checked and it isn't a valid socket,
	// return immediately.
	if (fe->type != TYPE_SPIPE) {
		switch (fe->type) {
		case TYPE_FILE:
			return 0;
		case TYPE_BADFD:
#ifdef SYS_UNIX
			acl_fiber_set_error(EBADF);
#endif
			msg_error("%s(%d): invalid fd=%d", __FUNCTION__,
				__LINE__, (int) fe->fd);
			return -1;
		default:
#ifdef SYS_UNIX
			acl_fiber_set_error(EINVAL);
#endif
			msg_error("%s(%d): invalid type=%d, fd=%d",
				__FUNCTION__, __LINE__, fe->type, (int) fe->fd);
			return -1;
		}
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		msg_error("%s(%d): fd=%d >= setsize=%d", __FUNCTION__,
			__LINE__, fe->fd, (int) ev->setsize);
		acl_fiber_set_error(ERANGE);
		return 0;
	}

	if (fe->oper & EVENT_DEL_READ) {
		fe->oper &= ~EVENT_DEL_READ;
	}

	if (!(fe->mask & EVENT_READ)) {
		// we should check the fd's type for the first time.
		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_ADD_READ;
	}

	fe->r_proc = proc;
	return 1;
}

int event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	if (fe->type == TYPE_NONE) {
		int ret = event_checkfd(ev, fe);
		if (ret <= 0) {
			return ret;
		}
	}

	if (fe->type != TYPE_SPIPE) {
		switch (fe->type) {
		case TYPE_FILE:
			return 0;
		case TYPE_BADFD:
			return -1;
		default:
			return -1;
		}
	}

	if (fe->fd >= (socket_t) ev->setsize) {
		msg_error("%s(%d): fd=%d >= setsize=%d", __FUNCTION__,
			__LINE__, fe->fd, (int) ev->setsize);
		acl_fiber_set_error(ERANGE);
		return 0;
	}

	if (fe->oper & EVENT_DEL_WRITE) {
		fe->oper &= ~EVENT_DEL_WRITE;
	}

	if (!(fe->mask & EVENT_WRITE)) {
		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_ADD_WRITE;
	}

	fe->w_proc = proc;
	return 1;
}

void event_del_read(EVENT *ev, FILE_EVENT *fe)
{
	if (fe->oper & EVENT_ADD_READ) {
		fe->oper &=~EVENT_ADD_READ;
	}

	if (fe->mask & EVENT_READ) {
		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_DEL_READ;
	}

	fe->r_proc  = NULL;
}

void event_del_write(EVENT *ev, FILE_EVENT *fe)
{
	if (fe->oper & EVENT_ADD_WRITE) {
		fe->oper &= ~EVENT_ADD_WRITE;
	}

	if (fe->mask & EVENT_WRITE) {
		if (fe->me.parent == &fe->me) {
			ring_prepend(&ev->events, &fe->me);
		}

		fe->oper |= EVENT_DEL_WRITE;
	}

	fe->w_proc = NULL;
}

void event_close(EVENT *ev, FILE_EVENT *fe)
{
	if (fe->mask & EVENT_READ) {
		ev->del_read(ev, fe);
	}

	if (fe->mask & EVENT_WRITE) {
		ev->del_write(ev, fe);
	}

	/* when one fiber add read/write and del read/write by another fiber
	 * in one loop, the fe->mask maybe be 0 and the fiber's fe maybe been
	 * added into events task list
	 */
	if (fe->me.parent != &fe->me) {
		ring_detach(&fe->me);
	}

	if (ev->event_fflush) {
		ev->event_fflush(ev);
	}
}

static void event_prepare(EVENT *ev)
{
	FILE_EVENT *fe;
	RING *next;

	while ((next = ring_first(&ev->events))) {
		fe = ring_to_appl(next, FILE_EVENT, me);

		if (fe->oper & EVENT_DEL_READ) {
			ev->del_read(ev, fe);
		}
		if (fe->oper & EVENT_DEL_WRITE) {
			ev->del_write(ev, fe);
		}
		if (fe->oper & EVENT_ADD_READ) {
			ev->add_read(ev, fe);
		}
		if (fe->oper & EVENT_ADD_WRITE) {
			ev->add_write(ev, fe);
		}

		ring_detach(next);
		fe->oper = 0;
	}

	ring_init(&ev->events);
}

#define TO_APPL	ring_to_appl

#ifdef HAS_POLL
static void event_process_poll(EVENT *ev)
{
	RING_ITER iter;
	RING *head;
	POLL_EVENT *pe;
	long long now = event_get_stamp(ev);
	TIMER_CACHE_NODE *node = avl_first(&ev->poll_list->tree), *next;

	/* Check and call all the pe's callback which was timeout except the
	 * pe which has been ready and been removed from ev->poll_list. The
	 * removing operations are in read_callback or write_callback in the
	 * hook/poll.c.
	 */
	while (node && node->expire >= 0 && node->expire <= now) {
		next = AVL_NEXT(&ev->poll_list->tree, node);

		// Call all the pe's callback with the same expire time.
		ring_foreach(iter, &node->ring) {
			pe = TO_APPL(iter.ptr, POLL_EVENT, me);
			pe->proc(ev, pe);
		}

		node = next;
	}

	while ((head = ring_pop_head(&ev->poll_ready)) != NULL) {
		pe = TO_APPL(head, POLL_EVENT, me);
		pe->proc(ev, pe);
	}

	ring_init(&ev->poll_ready);
}
#endif

#ifdef	HAS_EPOLL
static void event_process_epoll(EVENT *ev)
{
	RING_ITER iter;
	RING *head;
	EPOLL_EVENT *ee;
	long long now = event_get_stamp(ev);
	TIMER_CACHE_NODE *node = avl_first(&ev->epoll_list->tree), *next;

	while (node && node->expire >= 0 && node->expire <= now) {
		next = AVL_NEXT(&ev->epoll_list->tree, node);

		ring_foreach(iter, &node->ring) {
			ee = TO_APPL(iter.ptr, EPOLL_EVENT, me);
			ee->proc(ev, ee);
		}

		node = next;
	}

	while ((head = ring_pop_head(&ev->epoll_ready)) != NULL) {
		ee = TO_APPL(head, EPOLL_EVENT, me);
		ee->proc(ev, ee);
	}

	ring_init(&ev->epoll_ready);
}
#endif

int event_process(EVENT *ev, int timeout)
{
	int ret;

	if (ev->timeout < 0) {
		if (timeout < 0) {
			timeout = 100;
		}
	} else if (timeout < 0) {
		timeout = ev->timeout;
	} else if (timeout > ev->timeout) {
		timeout = ev->timeout;
	}

	/* limit the event wait time just for fiber schedule exiting
	 * quickly when no tasks left
	 */
	if (timeout > 1000 || timeout < 0) {
		timeout = 100;
	}

	event_prepare(ev);

	// call the system event waiting API for any event arriving.
	ret = ev->event_wait(ev, timeout);

	(void) event_set_stamp(ev);  // reset the stamp after event waiting.

#ifdef HAS_POLL
	event_process_poll(ev);
#endif

#ifdef	HAS_EPOLL
	event_process_epoll(ev);
#endif

	return ret;
}

