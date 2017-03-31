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

//#define DEBUG

#ifdef DEBUG
# define ASSERT assert
#else
# define ASSERT (void)
#endif

EVENT *event_create(int size)
{
	int i;
	EVENT *ev   = event_epoll_create(size);

	ev->events   = (FILE_EVENT *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->r_defers = (DEFER_DELETE *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->w_defers = (DEFER_DELETE *) acl_mycalloc(size, sizeof(FILE_EVENT));
	ev->fired    = (FIRED_EVENT *) acl_mycalloc(size, sizeof(FIRED_EVENT));
	ev->timeout  = -1;
	ev->setsize  = size;
	ev->maxfd    = -1;
	ev->r_ndefer = 0;
	ev->w_ndefer = 0;
	acl_ring_init(&ev->poll_list);
	acl_ring_init(&ev->epoll_list);

	/* Events with mask == AE_NONE are not set. So let's initialize the
	 * vector with it.
	 */
	for (i = 0; i < size; i++) {
		ev->events[i].mask       = EVENT_NONE;
		ev->events[i].mask_fired = EVENT_NONE;
		ev->events[i].r_defer    = NULL;
		ev->events[i].w_defer    = NULL;
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
	FILE_EVENT   *events   = ev->events;
	FIRED_EVENT  *fired    = ev->fired;
	DEFER_DELETE *r_defers = ev->r_defers;
	DEFER_DELETE *w_defers = ev->w_defers;

	ev->free(ev);

	acl_myfree(events);
	acl_myfree(r_defers);
	acl_myfree(w_defers);
	acl_myfree(fired);
}

static int check_fdtype(int fd)
{
	struct stat s;

	if (fstat(fd, &s) < 0) {
		acl_msg_info("%s(%d), %s: fd: %d fstat error %s", __FILE__,
			__LINE__, __FUNCTION__, fd, acl_last_serror());
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

#define DEL_DELAY

#ifdef DEL_DELAY
static int event_defer_r_merge(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe = &ev->events[fd];
	int fd2, pos = fe->r_defer->pos;
	int to_mask = mask | (fe->mask & ~(ev->r_defers[pos].mask));

	ASSERT(to_mask != 0);

	ev->r_ndefer--;
	ASSERT(ev->r_ndefer >= 0);

	fd2 = ev->r_defers[ev->r_ndefer].fd;

	if (ev->r_ndefer > 0) {
		ev->r_defers[pos].mask  = ev->r_defers[ev->r_ndefer].mask;
		ev->r_defers[pos].pos   = pos;
		ev->r_defers[pos].fd    = fd2;

		ev->events[fd2].r_defer = &ev->r_defers[pos];
	} else {
		if (fd2 >= 0)
			ev->events[fd2].r_defer = NULL;
		ev->r_defers[0].mask = EVENT_NONE;
		ev->r_defers[0].pos  = 0;
	}

	if (ev->add(ev, fd, to_mask) == -1) {
		acl_msg_error("mod fd(%d) error: %s", fd, acl_last_serror());
		return -1;
	}

	ev->r_defers[ev->r_ndefer].fd  = -1;
	fe->r_defer = NULL;
	fe->mask    = to_mask;
	return 0;
}

static int event_defer_w_merge(EVENT *ev, int fd, int mask)
{
	FILE_EVENT *fe = &ev->events[fd];
	int fd2, pos = fe->w_defer->pos;
	int to_mask = mask | (fe->mask & ~(ev->w_defers[pos].mask));

	ASSERT(to_mask != 0);

	ev->w_ndefer--;
	fd2 = ev->w_defers[ev->w_ndefer].fd;

	if (ev->w_ndefer > 0) {
		ev->w_defers[pos].mask  = ev->w_defers[ev->w_ndefer].mask;
		ev->w_defers[pos].pos   = pos;
		ev->w_defers[pos].fd    = fd2;

		ev->events[fd2].w_defer = &ev->w_defers[pos];
	} else {
		if (fd2 >= 0)
			ev->events[fd2].w_defer = NULL;
		ev->w_defers[0].mask = EVENT_NONE;
		ev->w_defers[0].pos  = 0;
	}

	if (ev->add(ev, fd, to_mask) == -1) {
		acl_msg_error("mod fd(%d) error: %s", fd, acl_last_serror());
		return -1;
	}

	ev->w_defers[ev->w_ndefer].fd  = -1;
	fe->w_defer = NULL;
	fe->mask    = to_mask;
	return 0;
}
#endif /* !DEL_DELAY */

int event_add(EVENT *ev, int fd, int mask, event_proc *proc, void *ctx)
{
	FILE_EVENT *fe;
	int nmerged = 0;

	if (fd >= ev->setsize) {
		acl_msg_error("fd: %d >= setsize: %d", fd, ev->setsize);
		errno = ERANGE;
		return -1;
	}

	fe = &ev->events[fd];

	if (fe->type == TYPE_NOSOCK)
		return 0;
	else if (fe->type == TYPE_NONE) {
		if (check_fdtype(fd) == 0)
			fe->type = TYPE_SOCK;
		else {
			fe->type = TYPE_NOSOCK;
			return 0;
		}
	}

#ifdef	DEL_DELAY
	if ((mask & EVENT_READABLE) && fe->r_defer != NULL) {
		if (event_defer_r_merge(ev, fd, mask) < 0)
			return -1;
		else
			nmerged++;
	}

	if ((mask & EVENT_WRITABLE) && fe->w_defer != NULL) {
		if (event_defer_w_merge(ev, fd, mask) < 0)
			return -1;
		else
			nmerged++;
	}
#endif

	if (nmerged == 0) {
		if (ev->add(ev, fd, mask) == -1) {
			acl_msg_error("add fd(%d) error: %s",
				fd, acl_last_serror());
			return -1;
		}

		fe->mask |= mask;
	}

	if (mask & EVENT_READABLE) {
		fe->r_proc = proc;
		fe->r_ctx  = ctx;
	}

	if (mask & EVENT_WRITABLE) {
		fe->w_proc = proc;
		fe->w_ctx  = ctx;
	}

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
		fe->mask_fired = EVENT_NONE;
		fe->r_defer    = NULL;
		fe->w_defer    = NULL;
		fe->pe         = NULL;
	} else if (ev->del(ev, fd, mask) == 1) {
		fe->mask_fired = EVENT_NONE;
		fe->type       = TYPE_NONE;
		fe->pe         = NULL;
		fe->mask       = fe->mask & (~mask);
	} else
		fe->mask       = fe->mask & (~mask);

	if (fd == ev->maxfd && fe->mask == EVENT_NONE) {
		/* Update the max fd */
		int j;

		for (j = ev->maxfd - 1; j >= 0; j--)
			if (ev->events[j].mask != EVENT_NONE)
				break;
		ev->maxfd = j;
	}
}

#ifdef	DEL_DELAY

static void event_defer_r_del(EVENT *ev, FILE_EVENT *fe)
{
	int fd;

	ev->r_ndefer--;
	ASSERT(ev->r_ndefer >= 0);

	fd = ev->r_defers[ev->r_ndefer].fd;

	if (ev->r_ndefer > 0) {
		int pos = fe->r_defer->pos;

		ev->r_defers[pos].mask = ev->r_defers[ev->r_ndefer].mask;
		ev->r_defers[pos].pos  = fe->r_defer->pos;
		ev->r_defers[pos].fd   = fd;

		/* move the last item here */
		ev->events[fd].r_defer = &ev->r_defers[pos];
	} else {
		if (fd >= 0)
			ev->events[fd].r_defer = NULL;
		ev->r_defers[0].mask = EVENT_NONE;
		ev->r_defers[0].pos = 0;
	}

	ev->r_defers[ev->r_ndefer].fd  = -1;
	fe->r_defer = NULL;
}

static void event_defer_w_del(EVENT *ev, FILE_EVENT *fe)
{
	int fd;

	ev->w_ndefer--;
	fd = ev->w_defers[ev->w_ndefer].fd;

	if (ev->w_ndefer > 0) {
		int pos = fe->w_defer->pos;

		ev->w_defers[pos].mask = ev->w_defers[ev->w_ndefer].mask;
		ev->w_defers[pos].pos  = fe->w_defer->pos;
		ev->w_defers[pos].fd   = fd;

		/* move the last item here */
		ev->events[fd].w_defer = &ev->w_defers[pos];
	} else {
		if (fd >= 0)
			ev->events[fd].w_defer = NULL;
		ev->w_defers[0].mask = EVENT_NONE;
		ev->w_defers[0].pos = 0;
	}

	ev->w_defers[ev->w_ndefer].fd  = -1;
	fe->w_defer = NULL;
}

static void event_error_del(EVENT *ev, int fd)
{
	FILE_EVENT *fe = &ev->events[fd];

	if (fe->r_defer != NULL)
		event_defer_r_del(ev, fe);
	if (fe->w_defer != NULL)
		event_defer_w_del(ev, fe);

	__event_del(ev, fd, fe->mask);
}

static void event_defer_r_add(EVENT *ev, int fd)
{
	ev->r_defers[ev->r_ndefer].fd   = fd;
	ev->r_defers[ev->r_ndefer].mask = EVENT_READABLE;
	ev->r_defers[ev->r_ndefer].pos  = ev->r_ndefer;

	ev->events[fd].r_defer = &ev->r_defers[ev->r_ndefer];
	ev->r_ndefer++;
}

static void event_defer_w_add(EVENT *ev, int fd)
{
	ev->w_defers[ev->w_ndefer].fd   = fd;
	ev->w_defers[ev->w_ndefer].mask = EVENT_WRITABLE;
	ev->w_defers[ev->w_ndefer].pos  = ev->w_ndefer;

	ev->events[fd].w_defer = &ev->w_defers[ev->w_ndefer];
	ev->w_ndefer++;
}

void event_del(EVENT *ev, int fd, int mask)
{
	if (ev->events[fd].type == TYPE_NOSOCK)
		ev->events[fd].type = TYPE_NONE;
	else if ((mask & EVENT_ERROR) != 0)
		event_error_del(ev, fd);
	else {
		if (mask & EVENT_READABLE)
			event_defer_r_add(ev, fd);
		if (mask & EVENT_WRITABLE)
			event_defer_w_add(ev, fd);
	}
}

#else

void event_del(EVENT *ev, int fd, int mask)
{
	if (ev->events[fd].type == TYPE_NOSOCK)
		ev->events[fd].type = TYPE_NONE;
	else
		__event_del(ev, fd, mask);
}

#endif /* !DEL_DELAY */

int event_process(EVENT *ev, int timeout)
{
	int processed = 0, numevents, j;
	int mask, fd, rfired;
	FILE_EVENT *fe;
	ACL_RING *head;
#ifdef	DEL_DELAY
	int ndefer;
#endif

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

#ifdef	DEL_DELAY
	ndefer = ev->r_ndefer;

	for (j = 0; j < ndefer; j++) {
		__event_del(ev, ev->r_defers[j].fd, ev->r_defers[j].mask);
		ev->events[ev->r_defers[j].fd].r_defer = NULL;
		ev->r_defers[j].fd = -1;
		ev->r_ndefer--;
	}
	ASSERT(ev->r_ndefer == 0);

	ndefer = ev->w_ndefer;
	for (j = 0; j < ndefer; j++) {
		__event_del(ev, ev->w_defers[j].fd, ev->w_defers[j].mask);
		ev->events[ev->w_defers[j].fd].w_defer = NULL;
		ev->w_defers[j].fd = -1;
		ev->w_ndefer--;
	}
	ASSERT(ev->w_ndefer == 0);
#endif

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
			fe->r_proc(ev, fd, fe->r_ctx, EVENT_READABLE);
		} else
			rfired = 0;

		if (fe->mask & mask & EVENT_WRITABLE) {
			if (!rfired || fe->w_proc != fe->r_proc)
				fe->w_proc(ev, fd, fe->w_ctx, EVENT_WRITABLE);
		}

		processed++;
	}

#define TO_APPL	acl_ring_to_appl

	while ((head = acl_ring_pop_head(&ev->poll_list))) {
		POLL_EVENT *pe = TO_APPL(head, POLL_EVENT, me);
		pe->proc(ev, pe);
		processed++;
	}

	while ((head = acl_ring_pop_head(&ev->epoll_list))) {
		EPOLL_EVENT *ee = TO_APPL(head, EPOLL_EVENT, me);

		ee->proc(ev, ee);
		processed++;
	}

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
