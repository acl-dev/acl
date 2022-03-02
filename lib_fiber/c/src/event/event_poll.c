#include "stdafx.h"
#include "common.h"

#ifdef HAS_POLL

#include "hook/hook.h"
#include "event.h"
#include "event_poll.h"

typedef struct EVENT_POLL {
	EVENT  event;
	FILE_EVENT **files;
	int    size;
	int    count;
	struct pollfd *pfds;
#ifdef	DELAY_CALL
	ARRAY *r_ready;
	ARRAY *w_ready;
#endif
} EVENT_POLL;

static void poll_free(EVENT *ev)
{
	EVENT_POLL *ep = (EVENT_POLL *) ev;

	mem_free(ep->files);
	mem_free(ep->pfds);
#ifdef	DELAY_CALL
	array_free(ep->r_ready, NULL);
	array_free(ep->w_ready, NULL);
#endif
	mem_free(ep);
}

static int poll_add_read(EVENT_POLL *ep, FILE_EVENT *fe)
{
	struct pollfd *pfd;

	if (fe->id == -1) {
		assert(ep->count < ep->size);
		fe->id = ep->count++;
	}

	pfd = &ep->pfds[fe->id];

	if (pfd->events & (POLLIN | POLLOUT)) {
		assert(ep->files[fe->id] == fe);
	} else {
		pfd->events       = 0;
		pfd->fd           = fe->fd;
		pfd->revents      = 0;
		ep->files[fe->id] = fe;
		ep->event.fdcount++;
	}

	fe->mask    |= EVENT_READ;
	pfd->events |= POLLIN;
	return 0;
}

static int poll_add_write(EVENT_POLL *ep, FILE_EVENT *fe)
{
	struct pollfd *pfd = (fe->id >= 0 && fe->id < ep->count)
		? &ep->pfds[fe->id] : NULL;

	if (fe->id == -1) {
		assert(ep->count < ep->size);
		fe->id = ep->count++;
	}

	pfd = &ep->pfds[fe->id];

	if (pfd->events & (POLLIN | POLLOUT)) {
		assert(ep->files[fe->id] == fe);
	} else {
		pfd->events       = 0;
		pfd->fd           = fe->fd;
		pfd->revents      = 0;
		ep->files[fe->id] = fe;
		ep->event.fdcount++;
	}

	fe->mask    |= EVENT_WRITE;
	pfd->events |= POLLOUT;
	return 0;
}

static int poll_del_read(EVENT_POLL *ep, FILE_EVENT *fe)
{
	struct pollfd *pfd;

	assert(fe->id >= 0 && fe->id < ep->count);
	pfd = &ep->pfds[fe->id];
	assert(pfd);

	if (pfd->events & POLLIN) {
		pfd->events &= ~POLLIN;
	}
	if (!(pfd->events & POLLOUT)) {
		if (fe->id < --ep->count) {
			ep->pfds[fe->id]      = ep->pfds[ep->count];
			ep->files[fe->id]     = ep->files[ep->count];
			ep->files[fe->id]->id = fe->id;
		}
		ep->pfds[ep->count].fd      = -1;
		ep->pfds[ep->count].events  = 0;
		ep->pfds[ep->count].revents = 0;
		fe->id = -1;
		ep->event.fdcount--;
	}
	fe->mask &= ~EVENT_READ;
	return 0;
}

static int poll_del_write(EVENT_POLL *ep, FILE_EVENT *fe)
{
	struct pollfd *pfd;

	assert(fe->id >= 0 && fe->id < ep->count);
	pfd = &ep->pfds[fe->id];
	assert(pfd);

	if (pfd->events & POLLOUT) {
		pfd->events &= ~POLLOUT;
	}
	if (!(pfd->events & POLLIN)) {
		if (fe->id < --ep->count) {
			ep->pfds[fe->id]      = ep->pfds[ep->count];
			ep->files[fe->id]     = ep->files[ep->count];
			ep->files[fe->id]->id = fe->id;
		}
		ep->pfds[ep->count].fd      = -1;
		ep->pfds[ep->count].events  = 0;
		ep->pfds[ep->count].revents = 0;
		fe->id = -1;
		ep->event.fdcount--;
	}
	fe->mask &= ~EVENT_WRITE;
	return 0;
}

static int poll_wait(EVENT *ev, int timeout)
{
	EVENT_POLL *ep = (EVENT_POLL *) ev;
	FILE_EVENT *fe;
	struct pollfd *pfd;
#ifdef	DELAY_CALL
	ITER  iter;
#endif
	int n, i;

#ifdef SYS_WIN
	if (ev->fdcount == 0) {
		Sleep(timeout);
		return 0;
	}
#endif
	n = (*sys_poll)(ep->pfds, ep->count, timeout);
#ifdef SYS_WIN
	if (n == SOCKET_ERROR) {
#else
	if (n == -1) {
#endif
		if (acl_fiber_last_error() == FIBER_EINTR) {
			return 0;
		}
		msg_fatal("%s: poll error %s", __FUNCTION__, last_serror());
	} else if (n == 0) {
		return n;
	}

	for (i = 0; i < ep->count; i++) {
		fe  = ep->files[i];
		pfd = &ep->pfds[fe->id];

#define ERR	(POLLERR | POLLHUP | POLLNVAL)

		if (pfd->revents & (POLLIN | ERR) && fe->r_proc) {
			if (pfd->revents & POLLERR) {
				fe->mask |= EVENT_ERR;
			}
			if (pfd->revents & POLLHUP) {
				fe->mask |= EVENT_HUP;
			}
			if (pfd->revents & POLLNVAL) {
				fe->mask |= EVENT_NVAL;
			}

			CLR_READWAIT(fe);
#ifdef	DELAY_CALL
			array_append(ep->r_ready, fe);
#else
			fe->r_proc(ev, fe);
#endif
		}

		if (pfd->revents & (POLLOUT | ERR ) && fe->w_proc) {
			if (pfd->revents & POLLERR) {
				fe->mask |= EVENT_ERR;
			}
			if (pfd->revents & POLLHUP) {
				fe->mask |= EVENT_HUP;
			}
			if (pfd->revents & POLLNVAL) {
				fe->mask |= EVENT_NVAL;
			}

			CLR_WRITEWAIT(fe);
#ifdef	DELAY_CALL
			array_append(ep->w_ready, fe);
#else
			fe->w_proc(ev, fe);
#endif
		}
	}

#ifdef	DELAY_CALL
	foreach(iter, ep->r_ready) {
		fe = (FILE_EVENT *) iter.data;
		fe->r_proc(ev, fe);
	}

	foreach(iter, ep->w_ready) {
		fe = (FILE_EVENT *) iter.data;
		fe->w_proc(ev, fe);
	}

	array_clean(ep->r_ready, NULL);
	array_clean(ep->w_ready, NULL);
#endif

	return n;
}

static int poll_checkfd(EVENT *ev UNUSED, FILE_EVENT *fe UNUSED)
{
	return -1;
}

static acl_handle_t poll_handle(EVENT *ev)
{
	(void) ev;
	return (acl_handle_t)-1;
}

static const char *poll_name(void)
{
	return "poll";
}

EVENT *event_poll_create(int size)
{
	EVENT_POLL *ep = (EVENT_POLL *) mem_calloc(1, sizeof(EVENT_POLL));

	if (sys_poll == NULL) {
		hook_once();
		if (sys_poll == NULL) {
			msg_error("%s: sys_poll NULL", __FUNCTION__);
			return NULL;
		}
	}

	// override size with system open limit setting
	size      = open_limit(0);
	ep->size  = size;
	ep->pfds  = (struct pollfd *) mem_calloc(size, sizeof(struct pollfd));
	ep->files = (FILE_EVENT**) mem_calloc(size, sizeof(FILE_EVENT*));
	ep->count = 0;

#ifdef	DELAY_CALL
	ep->r_ready = array_create(100);
	ep->w_ready = array_create(100);
#endif

	ep->event.name   = poll_name;
	ep->event.handle = poll_handle;
	ep->event.free   = poll_free;

	ep->event.event_wait = poll_wait;
	ep->event.checkfd    = (event_oper *) poll_checkfd;
	ep->event.add_read   = (event_oper *) poll_add_read;
	ep->event.add_write  = (event_oper *) poll_add_write;
	ep->event.del_read   = (event_oper *) poll_del_read;
	ep->event.del_write  = (event_oper *) poll_del_write;

	return (EVENT*) ep;
}

#endif
