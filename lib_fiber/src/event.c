#include "stdafx.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <poll.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "event_epoll.h"
#include "event.h"

EVENT *event_create(int size)
{
	int i;
	EVENT *ev = event_epoll_create(size);

	ev->events = (FILE_EVENT *) acl_mymalloc(sizeof(FILE_EVENT) * size);
	ev->fired = (FIRED_EVENT *) acl_mymalloc(sizeof(FIRED_EVENT) * size);
	ev->setsize = size;
	ev->stop = 0;
	ev->maxfd = -1;

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
	ev->free(ev);
}

void event_stop(EVENT *ev)
{
	ev->stop = 1;
}

int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize) {
		errno = ERANGE;
		return -1;
	}

	fe = &ev->events[fd];

	if (ev->add(ev, fd, mask) == -1)
		return -1;

	fe->mask |= mask;
	if (mask & EVENT_READABLE)
		fe->r_proc = proc;
	if (mask & EVENT_WRITABLE)
		fe->w_proc = proc;

	fe->ctx = ctx;
	if (fd > ev->maxfd)
		ev->maxfd = fd;

	return 0;
}

void event_del(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe;

	if (fd >= ev->setsize)
		return;

	fe = &ev->events[fd];
	if (fe->mask == EVENT_NONE)
		return;

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

int event_mask(EVENT *ev, int fd)
{
	if (fd >= ev->setsize)
		return 0;

	return ev->events[fd].mask;
}

int event_process(EVENT *ev)
{
	int processed = 0, numevents, j;
	struct timeval tv, *tvp;

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	tvp = &tv;

	numevents = ev->loop(ev, tvp);

	for (j = 0; j < numevents; j++) {
		FILE_EVENT *fe = &ev->events[ev->fired[j].fd];
		int mask = ev->fired[j].mask;
		int fd = ev->fired[j].fd;
		int rfired = 0;

		/* note the fe->mask & mask & ... code: maybe an already
		 * processed event removed an element that fired and we
		 * still didn't processed, so we check if the event is
		 * still valid.
		 */
		if (fe->mask & mask & EVENT_READABLE) {
			rfired = 1;
			fe->r_proc(ev, fd, fe->ctx, mask);
		}

		if (fe->mask & mask & EVENT_WRITABLE) {
			if (!rfired || fe->w_proc != fe->r_proc)
				fe->w_proc(ev, fd, fe->ctx, mask);
		}

		processed++;
	}

	/* return the number of processed file/time events */
	return processed;
}

void event_loop(EVENT *ev)
{
	ev->stop = 0;

	while (!ev->stop)
		event_process(ev);
}

/* Wait for milliseconds until the given file descriptor becomes
 * writable/readable/exception */
int event_wait(int fd, int mask, long long milliseconds)
{
	struct pollfd pfd;
	int retmask = 0, retval;

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	if (mask & EVENT_READABLE)
		pfd.events |= POLLIN;
	if (mask & EVENT_WRITABLE)
		pfd.events |= POLLOUT;

	if ((retval = poll(&pfd, 1, milliseconds))== 1) {
		if (pfd.revents & POLLIN)
			retmask |= EVENT_READABLE;
		if (pfd.revents & POLLOUT)
			retmask |= EVENT_WRITABLE;
		if (pfd.revents & POLLERR)
			retmask |= EVENT_WRITABLE;
		if (pfd.revents & POLLHUP)
			retmask |= EVENT_WRITABLE;

		return retmask;
	} else
		return retval;
}
