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
	} else {
		ev->enable_read(ev, fe, proc);
	}
}

void event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc)
{
	if (fe->fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fe->fd, ev->setsize);
		errno = ERANGE;
	} else {
		ev->enable_write(ev, fe, proc);
	}
}

void event_del_read(EVENT *ev, FILE_EVENT *fe)
{
	ev->disable_read(ev, fe);
}

void event_del_write(EVENT *ev, FILE_EVENT *fe)
{
	ev->disable_write(ev, fe);
}

int event_process(EVENT *ev, int timeout)
{
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

	return ev->event_loop(ev, timeout);
}
