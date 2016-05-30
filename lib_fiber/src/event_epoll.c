#include "stdafx.h"
#include <sys/epoll.h>
#include "event.h"
#include "event_epoll.h"

typedef struct EVENT_EPOLL {
	EVENT event;
	int epfd;
	struct epoll_event *epoll_events;
} EVENT_EPOLL;

static void __event_free(EVENT *ev)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;

	close(ep->epfd);
	acl_myfree(ep->epoll_events);
	acl_myfree(ep);
}

static int __event_add(EVENT *ev, int fd, int mask)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	struct epoll_event ee;
	/* If the fd was already monitored for some event, we need a MOD
	 * operation. Otherwise we need an ADD operation. */
	int op = ev->events[fd].mask == EVENT_NONE ?
		EPOLL_CTL_ADD : EPOLL_CTL_MOD;

	ee.events   = 0;
	ee.data.u64 = 0;
	ee.data.ptr = NULL;
	ee.data.fd  = fd;

	mask |= ev->events[fd].mask; /* Merge old events */
	if (mask & EVENT_READABLE)
		ee.events |= EPOLLIN;
	if (mask & EVENT_WRITABLE)
		ee.events |= EPOLLOUT;

	if (epoll_ctl(ep->epfd, op, fd, &ee) == -1) {
		acl_msg_error("%s, %s(%d): epoll_ctl error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
		return -1;
	}

	return 0;
}

static void __event_del(EVENT *ev, int fd, int delmask)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	struct epoll_event ee;
	int mask = ev->events[fd].mask & (~delmask);

	ee.events   = 0;
	ee.data.u64 = 0;
	ee.data.ptr = NULL;
	ee.data.fd  = fd;

	if (mask & EVENT_READABLE)
		ee.events |= EPOLLIN;
	if (mask & EVENT_WRITABLE)
		ee.events |= EPOLLOUT;

	if (mask != EVENT_NONE)
		epoll_ctl(ep->epfd, EPOLL_CTL_MOD, fd, &ee);
	else {
		/* Note, Kernel < 2.6.9 requires a non null event pointer
		 * even for EPOLL_CTL_DEL.
		 */
		epoll_ctl(ep->epfd, EPOLL_CTL_DEL, fd, &ee);
	}
}

static int __event_loop(EVENT *ev, struct timeval *tv)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) ev;
	int retval, numevents = 0;

	retval = epoll_wait(ep->epfd, ep->epoll_events, ev->setsize,
			tv ? (tv->tv_sec * 1000 + tv->tv_usec / 1000) : -1);
	if (retval > 0) {
		int j, mask;
		struct epoll_event *e;

		numevents = retval;
		for (j = 0; j < numevents; j++) {
			mask = 0;
			e = ep->epoll_events + j;

			if (e->events & EPOLLIN)
				mask |= EVENT_READABLE;
			if (e->events & EPOLLOUT)
				mask |= EVENT_WRITABLE;
			if (e->events & EPOLLERR)
				mask |= EVENT_WRITABLE;
			if (e->events & EPOLLHUP)
				mask |= EVENT_WRITABLE;

			ev->fired[j].fd = e->data.fd;
			ev->fired[j].mask = mask;
		}
	}

	return numevents;
}

static const char *__event_name(void)
{
	return "epoll";
}

EVENT *event_epoll_create(int setsize)
{
	EVENT_EPOLL *ep = (EVENT_EPOLL *) acl_mymalloc(sizeof(EVENT_EPOLL));

	ep->epoll_events = (struct epoll_event *)
		acl_mymalloc(sizeof(struct epoll_event) * setsize);

	ep->epfd = epoll_create(1024);
	acl_assert(ep->epfd >= 0);

	ep->event.name = __event_name;
	ep->event.loop = __event_loop;
	ep->event.add  = __event_add;
	ep->event.del  = __event_del;
	ep->event.free = __event_free;

	return (EVENT*) ep;
}
