#include "stdafx.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

#include "event/event_epoll.h"
#include "event.h"

EVENT *event_create(int size)
{
	EVENT *ev   = event_epoll_create(size);

	acl_ring_init(&ev->events);
	ev->timeout  = -1;
	ev->setsize  = size;
	ev->maxfd    = -1;

#ifdef	USE_RING
	acl_ring_init(&ev->poll_list);
	acl_ring_init(&ev->epoll_list);
#elif	defined(USE_STACK)
	ev->poll_list  = acl_stack_create(100);
	ev->epoll_list = acl_stack_create(100);
#else
	ev->poll_list  = acl_fifo_new();
	ev->epoll_list = acl_fifo_new();
#endif

	return ev;
}

const char *event_name(EVENT *ev)
{
	return ev->name();
}

int event_handle(EVENT *ev)
{
	return ev->handle(ev);
}

int event_size(EVENT *ev)
{
	return ev->setsize;
}

void event_free(EVENT *ev)
{
#if	defined(USE_STACK)
	acl_stack_destroy(ev->poll_list, NULL);
	acl_stack_destroy(ev->epoll_list, NULL);
#elif	!defined(USE_RING)
	acl_fifo_free(ev->poll_list, NULL);
	acl_fifo_free(ev->epoll_list, NULL);
#endif

	ev->free(ev);
}

/*
static int check_fdtype(int fd)
{
	struct stat s;

	if (fstat(fd, &s) < 0) {
		acl_msg_info("%s(%d), %s: fd: %d fstat error %s", __FILE__,
			__LINE__, __FUNCTION__, fd, acl_last_serror());
		return -1;
	}

	if (S_ISSOCK(s.st_mode) || S_ISFIFO(s.st_mode)) {
		return 0;
	}

	if (S_ISCHR(s.st_mode) && isatty(fd)) {
		return 0;
	}

	return -1;
}
*/

void event_add_read(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	if (fe->fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fe->fd, ev->setsize);
		errno = ERANGE;
	} else if (fe->oper & EVENT_DEL_READ) {
		fe->oper &= ~EVENT_DEL_READ;
	} else if (fe->oper == 0) {
		acl_ring_prepend(&ev->events, &fe->me);
	}

	if (!(fe->mask & EVENT_READ)) {
		fe->oper |= EVENT_ADD_READ;
	}
	fe->r_proc = proc;
}

void event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	if (fe->fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fe->fd, ev->setsize);
		errno = ERANGE;
	} else if (fe->oper & EVENT_DEL_WRITE) {
		fe->oper &= ~EVENT_DEL_WRITE;
	} else if (fe->oper == 0) {
		acl_ring_prepend(&ev->events, &fe->me);
	}

	if (!(fe->mask & EVENT_WRITE))
		fe->oper |= EVENT_ADD_WRITE;
	fe->w_proc = proc;
}

void event_del_read(EVENT *ev, FILE_EVENT *fe)
{
	if (fe->oper & EVENT_ADD_READ) {
		fe->oper &=~EVENT_ADD_READ;
	} else if (fe->oper == 0) {
		acl_ring_prepend(&ev->events, &fe->me);
	}

	if (fe->mask & EVENT_READ) {
		fe->oper |= EVENT_DEL_READ;
	}
	fe->r_proc  = NULL;
}

void event_del_write(EVENT *ev, FILE_EVENT *fe)
{
	if (fe->oper & EVENT_ADD_WRITE) {
		fe->oper &= ~EVENT_ADD_WRITE;
	} else if (fe->oper == 0) {
		acl_ring_prepend(&ev->events, &fe->me);
	}

	if (fe->mask & EVENT_WRITE)
		fe->oper |= EVENT_DEL_WRITE;
	fe->w_proc = NULL;
}

static void event_prepare(EVENT *ev)
{
	FILE_EVENT *fe;
	ACL_RING_ITER iter;

	acl_ring_foreach(iter, &ev->events) {
		fe = acl_ring_to_appl(iter.ptr, FILE_EVENT, me);

		if (fe->oper & EVENT_ADD_READ) {
			ev->add_read(ev, fe);
		}
		if (fe->oper & EVENT_ADD_WRITE) {
			ev->add_write(ev, fe);
		}
		if (fe->oper & EVENT_DEL_READ) {
			ev->del_read(ev, fe);
		}
		if (fe->oper & EVENT_DEL_WRITE) {
			ev->del_write(ev, fe);
		}

		fe->oper = 0;
	}

	acl_ring_init(&ev->events);
}

#define TO_APPL	acl_ring_to_appl

static inline void event_process_poll(EVENT *ev)
{
#ifdef	USE_RING
	while (1) {
		POLL_EVENT *pe;
		ACL_RING *head = acl_ring_pop_head(&ev->poll_list);
		if (head == NULL) {
			break;
		}
		pe = TO_APPL(head, POLL_EVENT, me);
		pe->proc(ev, pe);
	}
#elif	defined(USE_STACK)
	while (1) {
		POLL_EVENT *pe = acl_stack_pop(ev->poll_list);
		if (pe == NULL) {
			break;
		}
		pe->proc(ev, pe);
	}
#else
	while (1) {
		POLL_EVENT *pe = acl_fifo_pop(ev->poll_list);
		if (pe == NULL) {
			break;
		}
		pe->proc(ev, pe);
	}
#endif
}

static void event_process_epoll(EVENT *ev)
{
#ifdef	USE_RING
	while (1) {
		EPOLL_EVENT *ee;
		ACL_RING *head = acl_ring_pop_head(&ev->epoll_list);
		if (head == NULL) {
			break;
		}
		ee = TO_APPL(head, EPOLL_EVENT, me);
		ee->proc(ev, ee);
	}
#elif	defined(USE_STACK)
	while (1) {
		EPOLL_EVENT *ee = acl_stack_pop(ev->epoll_list);
		if (ee == NULL) {
			break;
		}
		ee->proc(ev, ee);
	}
#else
	while (1) {
		EPOLL_EVENT *ee = acl_fifo_pop(ev->epoll_list);
		if (ee == NULL) {
			break;
		}
		ee->proc(ev, ee);
	}
#endif
}

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
	if (timeout > 1000 || timeout <= 0) {
		timeout = 100;
	}

	event_prepare(ev);
	ret = ev->event_loop(ev, timeout);
	event_process_poll(ev);
	event_process_epoll(ev);

	return ret;
}
