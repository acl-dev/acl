#include "stdafx.h"
#include "common.h"

#include "event/event_epoll.h"
#include "event.h"

EVENT *event_create(int size)
{
	EVENT *ev = event_epoll_create(size);

	ring_init(&ev->events);
	ev->timeout  = -1;
	ev->setsize  = size;
	ev->maxfd    = -1;

	ring_init(&ev->poll_list);
	ring_init(&ev->epoll_list);

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
	ev->free(ev);
}

/*
static int check_fdtype(int fd)
{
	struct stat s;

	if (fstat(fd, &s) < 0) {
		msg_info("%s(%d), %s: fd: %d fstat error %s", __FILE__,
			__LINE__, __FUNCTION__, fd, last_serror());
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
		msg_error("fd: %d >= setsize: %d", fe->fd, ev->setsize);
		errno = ERANGE;
	} else if (fe->oper & EVENT_DEL_READ) {
		fe->oper &= ~EVENT_DEL_READ;
	} else if (fe->oper == 0) {
		ring_prepend(&ev->events, &fe->me);
	}

	if (!(fe->mask & EVENT_READ)) {
		fe->oper |= EVENT_ADD_READ;
	}
	fe->r_proc = proc;
}

void event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	if (fe->fd >= ev->setsize) {
		msg_error("fd: %d >= setsize: %d", fe->fd, ev->setsize);
		errno = ERANGE;
	} else if (fe->oper & EVENT_DEL_WRITE) {
		fe->oper &= ~EVENT_DEL_WRITE;
	} else if (fe->oper == 0) {
		ring_prepend(&ev->events, &fe->me);
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
		ring_prepend(&ev->events, &fe->me);
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
		ring_prepend(&ev->events, &fe->me);
	}

	if (fe->mask & EVENT_WRITE)
		fe->oper |= EVENT_DEL_WRITE;
	fe->w_proc = NULL;
}

static void event_prepare(EVENT *ev)
{
	FILE_EVENT *fe;
	RING_ITER iter;

	ring_foreach(iter, &ev->events) {
		fe = ring_to_appl(iter.ptr, FILE_EVENT, me);

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

	ring_init(&ev->events);
}

#define TO_APPL	ring_to_appl

static inline void event_process_poll(EVENT *ev)
{
	while (1) {
		POLL_EVENT *pe;
		RING *head = ring_pop_head(&ev->poll_list);
		if (head == NULL) {
			break;
		}
		pe = TO_APPL(head, POLL_EVENT, me);
		pe->proc(ev, pe);
	}
}

static void event_process_epoll(EVENT *ev)
{
	while (1) {
		EPOLL_EVENT *ee;
		RING *head = ring_pop_head(&ev->epoll_list);
		if (head == NULL) {
			break;
		}
		ee = TO_APPL(head, EPOLL_EVENT, me);
		ee->proc(ev, ee);
	}
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
