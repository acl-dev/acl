#include "stdafx.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <errno.h>

#include "event_epoll.h"
#include "event.h"

EVENT *event_create(int size)
{
	int i;
	EVENT *ev   = event_epoll_create(size);

	ev->events  = (FILE_EVENT *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->defers  = (DEFER_DELETE *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->fired   = (FIRED_EVENT *) acl_mycalloc(size, sizeof(FIRED_EVENT));
	ev->setsize = size;
	ev->maxfd   = -1;
	ev->ndefer  = 0;
	ev->timeout = -1;
	acl_ring_init(&ev->poll_list);
	acl_ring_init(&ev->epoll_list);

	/* Events with mask == AE_NONE are not set. So let's initialize the
	 * vector with it.
	 */
	for (i = 0; i < size; i++) {
		ev->events[i].mask        = EVENT_NONE;
		ev->events[i].mask_fired  = EVENT_NONE;
		ev->events[i].defer       = NULL;
	}

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

/* Return the current set size. */
int event_size(EVENT *ev)
{
	return ev->setsize;
}

void event_free(EVENT *ev)
{
	FILE_EVENT *events   = ev->events;
	DEFER_DELETE *defers = ev->defers;
	FIRED_EVENT *fired   = ev->fired;

	ev->free(ev);

	acl_myfree(events);
	acl_myfree(defers);
	acl_myfree(fired);
}

static int check_fdtype(int fd)
{
	struct stat s;

	if (fstat(fd, &s) < 0) {
		acl_msg_info("fd: %d fstat error", fd);
		return -1;
	}

	if (S_ISSOCK(s.st_mode) || S_ISFIFO(s.st_mode) || S_ISCHR(s.st_mode))
		return 0;

	/*
	if (S_ISLNK(s.st_mode))
		acl_msg_info("fd %d S_ISLNK", fd);
	else if (S_ISREG(s.st_mode))
		acl_msg_info("fd %d S_ISREG", fd);
	else if (S_ISDIR(s.st_mode))
		acl_msg_info("fd %d S_ISDIR", fd);
	else if (S_ISCHR(s.st_mode))
		acl_msg_info("fd %d S_ISCHR", fd);
	else if (S_ISBLK(s.st_mode))
		acl_msg_info("fd %d S_ISBLK", fd);
	else if (S_ISFIFO(s.st_mode))
		acl_msg_info("fd %d S_ISFIFO", fd);
	else if (S_ISSOCK(s.st_mode))
		acl_msg_info("fd %d S_ISSOCK", fd);
	else
		acl_msg_info("fd: %d, unknoiwn st_mode: %d", fd, s.st_mode);
	*/

	return -1;
}

int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return -1;
	}

	fe = &ev->events[fd];

	if (fe->defer != NULL) {
		int fd2, pos = fe->defer->pos;
		int to_mask = mask | (fe->mask & ~(ev->defers[pos].mask));

		assert(to_mask != 0);

		ev->ndefer--;
		fd2 = ev->defers[ev->ndefer].fd;

		if (ev->ndefer > 0) {
			ev->defers[pos].mask  = ev->defers[ev->ndefer].mask;
			ev->defers[pos].pos   = pos;
			ev->defers[pos].fd    = fd2;

			ev->events[fd2].defer = &ev->defers[pos];
		} else {
			if (fd2 >= 0)
				ev->events[fd2].defer = NULL;
			ev->defers[0].mask = EVENT_NONE;
			ev->defers[0].pos  = 0;
		}

		if (ev->add(ev, fd, to_mask) == -1) {
			acl_msg_error("mod fd(%d) error: %s",
				fd, acl_last_serror());
			return -1;
		}

		ev->defers[ev->ndefer].fd  = -1;
		fe->defer = NULL;
		fe->mask  = to_mask;
	} else {
		if (fe->type == TYPE_NONE) {
			if (check_fdtype(fd) < 0) {
				fe->type = TYPE_NOSOCK;
				return 0;
			}

			fe->type = TYPE_SOCK;
		} else if (fe->type == TYPE_NOSOCK)
			return 0;

		if (ev->add(ev, fd, mask) == -1) {
			acl_msg_error("add fd(%d) error: %s",
				fd, acl_last_serror());
			return -1;
		}

		fe->mask |= mask;
	}

	if (mask & EVENT_READABLE)
		fe->r_proc = proc;
	if (mask & EVENT_WRITABLE)
		fe->w_proc = proc;

	fe->ctx = ctx;

	if (fd > ev->maxfd)
		ev->maxfd = fd;

	return 1;
}

static void __event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}

	fe = &ev->events[fd];

	if (fe->mask == EVENT_NONE) {
		/* acl_msg_info("----mask NONE, fd: %d----", fd); */
		fe->mask_fired = EVENT_NONE;
		fe->defer      = NULL;
		fe->pe         = NULL;
	} else if (ev->del(ev, fd, mask) == 1) {
		fe->mask_fired = EVENT_NONE;
		fe->type       = TYPE_NONE;
		fe->defer      = NULL;
		fe->pe         = NULL;
		fe->mask = fe->mask & (~mask);
	} else
		fe->mask = fe->mask & (~mask);

	if (fd == ev->maxfd && fe->mask == EVENT_NONE) {
		/* Update the max fd */
		int j;

		for (j = ev->maxfd - 1; j >= 0; j--)
			if (ev->events[j].mask != EVENT_NONE)
				break;
		ev->maxfd = j;
	}
}

#define DEL_DELAY

void event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	fe = &ev->events[fd];

	if (fe->type == TYPE_NOSOCK) {
		fe->type = TYPE_NONE;
		return;
	}

#ifdef DEL_DELAY
	if ((mask & EVENT_ERROR) == 0 && (mask & EVENT_WRITABLE) == 0) {
		ev->defers[ev->ndefer].fd   = fd;
		ev->defers[ev->ndefer].mask = mask;
		ev->defers[ev->ndefer].pos  = ev->ndefer;
		ev->events[fd].defer        = &ev->defers[ev->ndefer];

		ev->ndefer++;
		return;
	}
#endif

	if (fe->defer != NULL) {
		int fd2;

		ev->ndefer--;
		fd2 = ev->defers[ev->ndefer].fd;

		if (ev->ndefer > 0) {
			int pos = fe->defer->pos;

			ev->defers[pos].mask  = ev->defers[ev->ndefer].mask;
			ev->defers[pos].pos   = fe->defer->pos;
			ev->defers[pos].fd    = fd2;

			ev->events[fd2].defer = &ev->defers[pos];
		} else {
			if (fd2 >= 0)
				ev->events[fd2].defer = NULL;
			ev->defers[0].mask = EVENT_NONE;
			ev->defers[0].pos = 0;
		}

		ev->defers[ev->ndefer].fd  = -1;
		fe->defer = NULL;
	}

#ifdef DEL_DELAY
	__event_del(ev, fd, fe->mask);
#else
	__event_del(ev, fd, mask);
#endif
}

int event_process(EVENT *ev, int timeout)
{
	int processed = 0, numevents, j;
	int mask, fd, rfired, ndefer;
	FILE_EVENT *fe;

	if (ev->timeout < 0) {
		if (timeout < 0)
			timeout = 100;
	} else if (timeout < 0)
		timeout = ev->timeout;
	else if (timeout > ev->timeout)
		timeout = ev->timeout;

	/* limit the event wait time just for fiber schedule exiting
	 * quickly when no tasks left
	 */
	if (timeout > 1000)
		timeout = 1000;

	ndefer = ev->ndefer;

	for (j = 0; j < ndefer; j++) {
		__event_del(ev, ev->defers[j].fd, ev->defers[j].mask);
		ev->events[ev->defers[j].fd].defer = NULL;
		ev->defers[j].fd = -1;
		ev->ndefer--;
	}

	numevents = ev->loop(ev, timeout);

	for (j = 0; j < numevents; j++) {
		fd             = ev->fired[j].fd;
		mask           = ev->fired[j].mask;
		fe             = &ev->events[fd];
		fe->mask_fired = mask;

		/* note the fe->mask & mask & ... code: maybe an already
		 * processed event removed an element that fired and we
		 * still didn't processed, so we check if the event is
		 * still valid.
		 */
		if (fe->mask & mask & EVENT_READABLE) {
			rfired = 1;
			fe->r_proc(ev, fd, fe->ctx, mask);
		} else
			rfired = 0;

		if (fe->mask & mask & EVENT_WRITABLE) {
			if (!rfired || fe->w_proc != fe->r_proc)
				fe->w_proc(ev, fd, fe->ctx, mask);
		}

		processed++;
	}

#define TO_APPL	acl_ring_to_appl

	acl_ring_foreach(ev->iter, &ev->poll_list) {
		POLL_EVENT *pe = TO_APPL(ev->iter.ptr, POLL_EVENT, me);

		pe->proc(ev, pe);
		processed++;
	}
	acl_ring_init(&ev->poll_list);

	acl_ring_foreach(ev->iter, &ev->epoll_list) {
		EPOLL_EVENT *ee = TO_APPL(ev->iter.ptr, EPOLL_EVENT, me);

		ee->proc(ev, ee);
		processed++;
	}
	acl_ring_init(&ev->epoll_list);

	/* return the number of processed file/time events */
	return processed;
}

int event_readable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return 0;
	}

	return ev->events[fd].mask_fired & EVENT_READABLE;
}

int event_writeable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return 0;
	}

	return ev->events[fd].mask_fired & EVENT_WRITABLE;
}

void event_clear_readable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}

	ev->events[fd].mask_fired &= ~EVENT_READABLE;
}

void event_clear_writeable(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}

	ev->events[fd].mask_fired &= ~ EVENT_WRITABLE;
}

void event_clear(EVENT *ev, int fd)
{
	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return;
	}
	
	ev->events[fd].mask_fired = EVENT_NONE;
}
