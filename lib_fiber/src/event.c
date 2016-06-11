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
	EVENT *ev = event_epoll_create(size);

	ev->events  = (FILE_EVENT *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->defers  = (DEFER_DELETE *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->fired   = (FIRED_EVENT *) acl_mycalloc(size, sizeof(FIRED_EVENT));
	ev->setsize = size;
	ev->maxfd   = -1;
	ev->ndefer  = 0;
	ev->timeout = -1;
	acl_ring_init(&ev->pevents_list);

	/* Events with mask == AE_NONE are not set. So let's initialize the
	 * vector with it.
	 */
	for (i = 0; i < size; i++)
		ev->events[i].mask = EVENT_NONE;

	return ev;
}

/* Return the current set size. */
int event_size(EVENT *ev)
{
	return ev->setsize;
}

void event_free(EVENT *ev)
{
	acl_myfree(ev->events);
	acl_myfree(ev->defers);
	acl_myfree(ev->fired);

	ev->free(ev);
}

void event_poll(EVENT *ev, POLL_EVENTS *pe, int timeout)
{
	int i;

	acl_ring_prepend(&ev->pevents_list, &pe->me);
	pe->nready = 0;
	for (i = 0; i < pe->nfds; i++) {
		if (pe->fds[i].events & POLLIN) {
			event_add(ev, pe->fds[i].fd, EVENT_READABLE, NULL, pe);
			ev->events[pe->fds[i].fd].pevents = pe;
			ev->events[pe->fds[i].fd].pfd = &pe->fds[i];
		}

		if (pe->fds[i].events & POLLOUT) {
			event_add(ev, pe->fds[i].fd, EVENT_WRITABLE, NULL, pe);
			ev->events[pe->fds[i].fd].pevents = pe;
			ev->events[pe->fds[i].fd].pfd = &pe->fds[i];
		}

		pe->fds[i].revents = 0;
	}

	if (timeout > 0) {
		if (ev->timeout < 0 || timeout < ev->timeout)
			ev->timeout = timeout;
	}
}

int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx)
{
	FILE_EVENT *fe;
	int curr = mask;

	if (fd >= ev->setsize) {
		errno = ERANGE;
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		return -1;
	}

	fe = &ev->events[fd];

	if (fe->defer != NULL) {
		int defer_fd, defer_pos = fe->defer->pos;

		ev->ndefer--;

		defer_fd = ev->defers[ev->ndefer].fd;

#ifdef _DEBUG
		if (ev->ndefer < 0 || fe->defer->fd == -1)
		{
			printf(">>>%s(%d)->ndefer: %d, "
				"pos: %d, %d, fd: %d, %d, %d\r\n",
				__FUNCTION__, __LINE__, ev->ndefer,
				ev->defers[fe->defer->pos].pos,
				fe->defer->pos, fe->defer->fd,
				ev->defers[fe->defer->pos].fd, fd);
			abort();
		}
#endif
		mask |= fe->mask & ~(ev->defers[defer_pos].mask);

		ev->defers[defer_pos].mask = ev->defers[ev->ndefer].mask;
		ev->defers[defer_pos].pos  = fe->defer->pos;
		ev->defers[defer_pos].fd   = defer_fd;

		ev->events[defer_fd].defer = &ev->defers[defer_pos];
		ev->defers[ev->ndefer].fd  = -1;
	}

	//printf("fe->mask: %d, mask: %d\r\n", fe->mask, curr);
	if ((fe->mask & curr) != curr && ev->add(ev, fd, mask) == -1) {
		acl_msg_error("add fd(%d) error: %s", fd, acl_last_serror());
		return -1;
	}

	if (fe->defer) {
		fe->defer = NULL;
		fe->mask  = mask;
	} else
		fe->mask |= mask;

	if (mask & EVENT_READABLE)
		fe->r_proc = proc;
	if (mask & EVENT_WRITABLE)
		fe->w_proc = proc;

	fe->ctx     = ctx;
	fe->pevents = NULL;
	fe->pfd     = NULL;

	if (fd > ev->maxfd)
		ev->maxfd = fd;

	return 0;
}

static void __event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize)
		return;

	fe          = &ev->events[fd];
	fe->defer   = NULL;
	fe->pevents = NULL;
	fe->pfd     = NULL;

	if (fe->mask == EVENT_NONE)
	{
		printf("mask NONE\r\n");
		return;
	}

	ev->del(ev, fd, mask);
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

void event_del(EVENT *ev, int fd, int mask)
{
#ifdef DEL_NOBUF
	__event_del(ev, fd, mask);
	return;
#endif

#ifdef _DEBUG
	int eq = ev->defers[ev->ndefer].fd == fd ? 1 : 0;
#endif
	ev->defers[ev->ndefer].fd   = fd;
	ev->defers[ev->ndefer].mask = mask;
	ev->defers[ev->ndefer].pos  = ev->ndefer;
	ev->events[fd].defer        = &ev->defers[ev->ndefer];

#ifdef _DEBUG
	if (!eq)
		ev->ndefer++;
	else
		printf("---fd: %d, eq: %s, pos: %d, ndefer: %d---\r\n",
			fd, eq ? "yes" : "no",
			ev->defers[ev->ndefer].pos, ev->ndefer);
#else
	ev->ndefer++;
#endif
}

int event_mask(EVENT *ev, int fd)
{
	if (fd >= ev->setsize)
		return 0;

	return ev->events[fd].mask;
}

int event_process(EVENT *ev, int left)
{
	int processed = 0, numevents, j;
	struct timeval tv, *tvp;
	int mask, fd, rfired, ndefer;
	FILE_EVENT *fe;

	if (ev->timeout < 0) {
		if (left < 0) {
			tv.tv_sec = 1;
			tv.tv_usec = 0;
		} else {
			tv.tv_sec  = left / 1000;
			tv.tv_usec = (left - tv.tv_sec * 1000) * 1000;
		}
	} else if (left < 0) {
		tv.tv_sec = ev->timeout / 1000;
		tv.tv_usec = (ev->timeout - tv.tv_sec * 1000) * 1000;
	} else if (left < ev->timeout) {
		tv.tv_sec  = left / 1000;
		tv.tv_usec = (left - tv.tv_sec * 1000) * 1000;
	} else {
		tv.tv_sec = ev->timeout / 1000;
		tv.tv_usec = (ev->timeout - tv.tv_sec * 1000) * 1000;
	}

	tvp = &tv;

	ndefer = ev->ndefer;

	for (j = 0; j < ndefer; j++) {
		__event_del(ev, ev->defers[j].fd, ev->defers[j].mask);
		ev->events[ev->defers[j].fd].defer = NULL;
		ev->defers[j].fd = -1;
		ev->ndefer--;
	}

	numevents = ev->loop(ev, tvp);

	for (j = 0; j < numevents; j++) {
		fd   = ev->fired[j].fd;
		mask = ev->fired[j].mask;
		fe   = &ev->events[fd];

		if (fe->pevents != NULL) {
			if (fe->mask & mask & EVENT_READABLE) {
				fe->pfd->revents |= POLLIN;
				fe->pevents->nready++;
			}

			if (fe->mask & mask & EVENT_WRITABLE) {
				fe->pfd->revents |= POLLOUT;
				fe->pevents->nready++;
			}

			continue;
		}

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

	acl_ring_foreach(ev->iter, &ev->pevents_list) {
		POLL_EVENTS *pe = acl_ring_to_appl(ev->iter.ptr,
				POLL_EVENTS, me);

		pe->proc(ev, pe);
		processed++;
	}

	acl_ring_init(&ev->pevents_list);

	/* return the number of processed file/time events */
	return processed;
}
