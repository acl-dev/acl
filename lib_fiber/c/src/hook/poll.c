#include "stdafx.h"
#include "common.h"

#include "event.h"
#include "fiber.h"
#include "hook.h"

#ifdef HAS_POLL

/****************************************************************************/

#define TO_APPL ring_to_appl

/**
 * The callback is set by poll_event_set() when user calls acl_fiber_poll().
 * The callback will be called when the fd included by FILE_EVENT is ready,
 * and POLLIN flag will be set in the specified FILE_EVENT that will be used
 * by the application called acl_fiber_poll().
 */
static void read_callback(EVENT *ev, FILE_EVENT *fe)
{
	POLLFD *pfd = fe->pfd;

	/* In iocp mode on windows, the pfd maybe be set NULL when more overlapped
	 * events happened by IO or poll events.
	 */
	if (pfd == NULL) {
		return;
	}

	assert(pfd->pfd->events & POLLIN);

	event_del_read(ev, fe);

	pfd->pfd->revents |= POLLIN;

	if (fe->mask & EVENT_ERR) {
		pfd->pfd->revents |= POLLERR;
	}
	if (fe->mask & EVENT_HUP) {
		pfd->pfd->revents |= POLLHUP;
	}
	if (fe->mask & EVENT_NVAL) {
		pfd->pfd->revents |= POLLNVAL;
	}

	if (!(pfd->pfd->events & POLLOUT)) {
		fe->pfd = NULL;
		pfd->fe = NULL;
	}

	assert(timer_cache_size(ev->poll_list) > 0);

	/*
	 * If any fe has been ready, the pe holding fe should be removed from
	 * ev->poll_list to avoid to be called in timeout process.
	 * We should just remove pe only once by checking if the value of
	 * pe->nready is 0. After the pe has been removed from the
	 * ev->poll_list, the pe's callback will not be called again in the
	 * timeout process in event_process_poll() in event.c.
	 */
	if (pfd->pe->nready == 0) {
		timer_cache_remove(ev->poll_list, pfd->pe->expire, &pfd->pe->me);
		ring_prepend(&ev->poll_ready, &pfd->pe->me);
	}
	pfd->pe->nready++;
	SET_READABLE(fe);
}

/**
 * Similiar to read_callback except that the POLLOUT flag will be set in it.
 */
static void write_callback(EVENT *ev, FILE_EVENT *fe)
{
	POLLFD *pfd = fe->pfd;

	if (pfd == NULL) {
		return;
	}

	assert(pfd->pfd->events & POLLOUT);

	event_del_write(ev, fe);

	pfd->pfd->revents |= POLLOUT;

	if (fe->mask & EVENT_ERR) {
		pfd->pfd->revents |= POLLERR;
	}
	if (fe->mask & EVENT_HUP) {
		pfd->pfd->revents |= POLLHUP;
	}
	if (fe->mask & EVENT_NVAL) {
		pfd->pfd->revents |= POLLNVAL;
	}

	if (!(pfd->pfd->events & POLLIN)) {
		fe->pfd = NULL;
		pfd->fe = NULL;
	}

	assert(timer_cache_size(ev->poll_list) > 0);

	if (pfd->pe->nready == 0) {
		timer_cache_remove(ev->poll_list, pfd->pe->expire, &pfd->pe->me);
		ring_prepend(&ev->poll_ready, &pfd->pe->me);
	}
	pfd->pe->nready++;
	SET_WRITABLE(fe);
}

/**
 * Set all fds' callbacks in POLL_EVENT, thease callbacks will be called
 * by event_wait() of different event engines for different OS platforms.
 */
static void poll_event_set(EVENT *ev, POLL_EVENT *pe, int timeout)
{
	int i;

	for (i = 0; i < pe->nfds; i++) {
		POLLFD *pfd = &pe->fds[i];

		if (pfd->pfd->events & POLLIN) {
			event_add_read(ev, pfd->fe, read_callback);
			SET_READWAIT(pfd->fe);
		}
		if (pfd->pfd->events & POLLOUT) {
			event_add_write(ev, pfd->fe, write_callback);
			SET_WRITEWAIT(pfd->fe);
		}

		pfd->fe->pfd      = pfd;
		pfd->pfd->revents = 0;
	}

	if (timeout >= 0) {
		pe->expire = event_get_stamp(ev) + timeout;
		if (ev->timeout < 0 || timeout < ev->timeout) {
			ev->timeout = timeout;
		}
	} else {
		pe->expire = -1;
	}
}

static void poll_event_clean(EVENT *ev, POLL_EVENT *pe)
{
	int i;

	for (i = 0; i < pe->nfds; i++) {
		POLLFD *pfd = &pe->fds[i];

		/* maybe has been cleaned in read_callback/write_callback */
		if (pfd->fe == NULL) {
			continue;
		}

		CLR_POLLING(pfd->fe);

		if (pfd->pfd->events & POLLIN) {
			CLR_READWAIT(pfd->fe);
			event_del_read(ev, pfd->fe);
		}
		if (pfd->pfd->events & POLLOUT) {
			CLR_WRITEWAIT(pfd->fe);
			event_del_write(ev, pfd->fe);
		}
		pfd->fe->pfd = NULL;
		pfd->fe      = NULL;
	}
}

/**
 * This callback will be called from event_process_poll() in event.c and the
 * fiber blocked after calling acl_fiber_switch() in acl_fiber_poll() will
 * wakeup and continue to run.
 */
static void poll_callback(EVENT *ev fiber_unused, POLL_EVENT *pe)
{
	fiber_io_dec();

	if (pe->fiber->status != FIBER_STATUS_READY) {
		acl_fiber_ready(pe->fiber);
	}
}

static POLLFD *pollfd_alloc(POLL_EVENT *pe, struct pollfd *fds, nfds_t nfds)
{
	POLLFD *pfds = (POLLFD *) mem_malloc(nfds * sizeof(POLLFD));
	nfds_t  i;

	for (i = 0; i < nfds; i++) {
		if (fds[i].events & POLLIN) {
			pfds[i].fe = fiber_file_open_read(fds[i].fd);
		} else {
			pfds[i].fe = fiber_file_open_write(fds[i].fd);
		}
#ifdef HAS_IOCP
		pfds[i].fe->buff = NULL;
		pfds[i].fe->size = 0;
#endif
		pfds[i].pe       = pe;
		pfds[i].pfd      = &fds[i];
		SET_POLLING(pfds[i].fe);
	}

	return pfds;
}

static void pollfd_free(POLLFD *pfds)
{
	mem_free(pfds);
}

int WINAPI acl_fiber_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	long long now;
	POLL_EVENT pe;
	EVENT *ev;
	int old_timeout;

	if (sys_poll == NULL) {
		hook_once();
	}

	if (!var_hook_sys_api) {
		return sys_poll ? (*sys_poll)(fds, nfds, timeout) : -1;
	}

	ev        = fiber_io_event();
	pe.fds    = pollfd_alloc(&pe, fds, nfds);
	pe.nfds   = nfds;
	pe.fiber  = acl_fiber_running();
	pe.proc   = poll_callback;
	pe.nready = 0;

	old_timeout = ev->timeout;
	poll_event_set(ev, &pe, timeout);
	ev->waiter++;

	while (1) {
		timer_cache_add(ev->poll_list, pe.expire, &pe.me);
		pe.nready = 0;

		fiber_io_inc();

		pe.fiber->status = FIBER_STATUS_POLL_WAIT;
		acl_fiber_switch();

		ev->timeout = old_timeout;

		if (acl_fiber_killed(pe.fiber)) {
			acl_fiber_set_error(pe.fiber->errnum);
			timer_cache_remove(ev->poll_list, pe.expire, &pe.me);
			pe.nready = -1;

			msg_info("%s(%d), %s: fiber-%u was killed, %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(pe.fiber), last_serror());
			break;
		}

		if (timer_cache_size(ev->poll_list) == 0) {
			ev->timeout = -1;
		}
		if (pe.nready != 0 || timeout == 0) {
			break;
		}

		now = event_get_stamp(ev);
		if (pe.expire > 0 && now >= pe.expire) {
			acl_fiber_set_error(FIBER_ETIMEDOUT);
			break;
		}
	}

	poll_event_clean(ev, &pe);
	pollfd_free(pe.fds);
	ev->waiter--;

	return pe.nready;
}

#ifdef SYS_UNIX
int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	return acl_fiber_poll(fds, nfds, timeout);
}
#endif

#endif // HAS_POLL
