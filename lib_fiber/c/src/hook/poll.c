#include "stdafx.h"
#include "common.h"

#include "event.h"
#include "fiber.h"

#ifdef HAS_POLL

#ifdef SYS_WIN
typedef int (WINAPI *poll_fn)(struct pollfd *, nfds_t, int);
#else
typedef int (*poll_fn)(struct pollfd *, nfds_t, int);
#endif

static poll_fn __sys_poll = NULL;

static void hook_api(void)
{
#ifdef SYS_WIN
	__sys_poll = WSAPoll;
#else
	__sys_poll = (poll_fn) dlsym(RTLD_NEXT, "poll");
	assert(__sys_poll);
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

static void hook_init(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

/****************************************************************************/

#define TO_APPL ring_to_appl

static void read_callback(EVENT *ev, FILE_EVENT *fe)
{
	POLLFD *pfd = fe->pfd;

	assert(pfd->pfd->events & POLLIN);

	event_del_read(ev, fe);
	pfd->pfd->revents |= POLLIN;

	if (!(pfd->pfd->events & POLLOUT)) {
		fe->pfd = NULL;
		pfd->fe = NULL;
	}

	assert(ring_size(&ev->poll_list) > 0);
	SET_READABLE(fe);
	pfd->pe->nready++;
}

static void write_callback(EVENT *ev, FILE_EVENT *fe)
{
	POLLFD *pfd = fe->pfd;

	assert(pfd->pfd->events & POLLOUT);

	event_del_write(ev, fe);
	pfd->pfd->revents |= POLLOUT;

	if (!(pfd->pfd->events & POLLIN)) {
		fe->pfd = NULL;
		pfd->fe = NULL;
	}

	assert(ring_size(&ev->poll_list) > 0);
	fe->status |= STATUS_WRITABLE;
	SET_WRITABLE(fe);
	pfd->pe->nready++;
}

static void poll_event_set(EVENT *ev, POLL_EVENT *pe, int timeout)
{
	int i;

	for (i = 0; i < pe->nfds; i++) {
		POLLFD *pfd = &pe->fds[i];

		if (pfd->pfd->events & POLLIN) {
			event_add_read(ev, pfd->fe, read_callback);
		}
		if (pfd->pfd->events & POLLOUT) {
			event_add_write(ev, pfd->fe, write_callback);
		}

		pfd->fe->pfd      = pfd;
		pfd->pfd->revents = 0;
	}

	if (timeout >= 0 && (ev->timeout < 0 || timeout < ev->timeout)) {
		ev->timeout = timeout;
	}
}

static void poll_event_clean(EVENT *ev, POLL_EVENT *pe)
{
	int i;

	for (i = 0; i < pe->nfds; i++) {
		POLLFD *pfd = &pe->fds[i];

		// maybe has been cleaned in read_callback/write_callback
		if (pfd->fe == NULL)
			continue;

		if (pfd->pfd->events & POLLIN) {
			event_del_read(ev, pfd->fe);
		}
		if (pfd->pfd->events & POLLOUT) {
			event_del_write(ev, pfd->fe);
		}
		pfd->fe->pfd = NULL;
		pfd->fe      = NULL;
	}
}

static void poll_callback(EVENT *ev fiber_unused, POLL_EVENT *pe)
{
	fiber_io_dec();
	acl_fiber_ready(pe->fiber);
}

static POLLFD *pollfd_alloc(POLL_EVENT *pe, struct pollfd *fds, nfds_t nfds)
{
	POLLFD *pfds = (POLLFD *) mem_malloc(nfds * sizeof(POLLFD));
	nfds_t  i;

	for (i = 0; i < nfds; i++) {
		pfds[i].fe  = fiber_file_open(fds[i].fd);
		pfds[i].pe  = pe;
		pfds[i].pfd = &fds[i];
	}

	return pfds;
}

static void pollfd_free(POLLFD *pfds)
{
	mem_free(pfds);
}

int WINAPI acl_fiber_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	long long begin, now;
	POLL_EVENT pe;
	EVENT *ev;

	if (__sys_poll == NULL) {
		hook_init();
	}

	if (!var_hook_sys_api) {
		return __sys_poll ? __sys_poll(fds, nfds, timeout) : -1;
	}

	ev        = fiber_io_event();
	pe.fds    = pollfd_alloc(&pe, fds, nfds);
	pe.nfds   = nfds;
	pe.fiber  = acl_fiber_running();
	pe.proc   = poll_callback;
	pe.nready = 0;

	poll_event_set(ev, &pe, timeout);
	SET_TIME(begin);
	ev->waiter++;

	while (1) {
		ring_prepend(&ev->poll_list, &pe.me);
		pe.nready = 0;

		fiber_io_inc();
		acl_fiber_switch();

		if (acl_fiber_killed(pe.fiber)) {
			ring_detach(&pe.me);
			msg_info("%s(%d), %s: fiber-%u was killed, %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(pe.fiber), last_serror());
			pe.nready = -1;
			break;
		}

		if (ring_size(&ev->poll_list) == 0) {
			ev->timeout = -1;
		}
		if (pe.nready != 0 || timeout == 0) {
			break;
		}
		SET_TIME(now);
		if (timeout > 0 && (now - begin >= timeout)) {
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
