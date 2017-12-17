#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "stdafx.h"
#include <dlfcn.h>
#include <poll.h>
#include <pthread.h>
#include "fiber/lib_fiber.h"
#include "event.h"
#include "fiber.h"

typedef int (*poll_fn)(struct pollfd *, nfds_t, int);

static poll_fn   __sys_poll   = NULL;

static void hook_poll(void)
{
	static acl_pthread_mutex_t __lock = PTHREAD_MUTEX_INITIALIZER;
	static int __called = 0;

	(void) acl_pthread_mutex_lock(&__lock);

	if (__called) {
		(void) acl_pthread_mutex_unlock(&__lock);
		return;
	}

	__called++;

	__sys_poll   = (poll_fn) dlsym(RTLD_NEXT, "poll");
	acl_assert(__sys_poll);

	(void) acl_pthread_mutex_unlock(&__lock);
}

/****************************************************************************/

#define SET_TIME(x) do { \
    struct timeval tv; \
    gettimeofday(&tv, NULL); \
    (x) = ((acl_int64) tv.tv_sec) * 1000 + ((acl_int64) tv.tv_usec)/ 1000; \
} while (0)

#define TO_APPL acl_ring_to_appl

/****************************************************************************/

static void read_callback(EVENT *ev, FILE_EVENT *fe)
{
	POLLFD *pfd        = (POLLFD *) fe;

	assert(pfd->pfd->events & POLLIN);

	event_del_read(ev, fe);
	pfd->pfd->revents |= POLLIN;

#ifdef	USE_RING
	assert(acl_ring_size(&ev->poll_list) > 0);
#endif

	pfd->pe->nready++;
}

static void write_callback(EVENT *ev, FILE_EVENT *fe)
{
	POLLFD *pfd        = (POLLFD *) fe;

	assert(pfd->pfd->events & POLLOUT);

	event_del_write(ev, fe);
	pfd->pfd->revents |= POLLOUT;

#ifdef	USE_RING
	assert(acl_ring_size(&ev->poll_list) > 0);
#endif
	pfd->pe->nready++;
}

static void event_poll_set(EVENT *ev, POLL_EVENT *pe, int timeout)
{
	int i;

#ifdef	USE_RING
	acl_ring_prepend(&ev->poll_list, &pe->me);
#elif	defined(USE_STACK)
	acl_stack_append(ev->poll_list, pe);
#else
	acl_fifo_push_back(ev->poll_list, pe);
#endif

	pe->nready = 0;

	for (i = 0; i < pe->nfds; i++) {
		POLLFD *pfd = &pe->fds[i];
		if (pfd->pfd->events & POLLIN) {
			event_add_read(ev, &pfd->fe, read_callback);
		}

		if (pfd->pfd->events & POLLOUT) {
			event_add_write(ev, &pfd->fe, write_callback);
		}

		pfd->pfd->revents = 0;
	}

	if (timeout >= 0 && (ev->timeout < 0 || timeout < ev->timeout)) {
		ev->timeout = timeout;
	}
}

static void event_poll_clear(EVENT *ev, POLL_EVENT *pe)
{
	int i;

	acl_ring_detach(&pe->me);

	for (i = 0; i < pe->nfds; i++) {
		POLLFD *pfd = &pe->fds[i];
		if (pfd->pfd->events & POLLIN) {
			event_del_read(ev, &pfd->fe);
		}
		if (pfd->pfd->events & POLLOUT) {
			event_del_write(ev, &pfd->fe);
		}
	}
}

static void poll_callback(EVENT *ev acl_unused, POLL_EVENT *pe)
{
	fiber_io_dec();
	acl_fiber_ready(pe->fiber);
}

static POLLFD *pollfd_alloc(POLL_EVENT *pe, struct pollfd *fds, nfds_t nfds)
{
	POLLFD *pfds = (POLLFD *) acl_mymalloc(nfds * sizeof(POLLFD));
	nfds_t  i;

	for (i = 0; i < nfds; i++) {
		file_event_init(&pfds[i].fe, fds[i].fd);
		pfds[i].pe  = pe;
		pfds[i].pfd = &fds[i];
	}

	return pfds;
}

static void pollfd_free(POLLFD *pfds)
{
	acl_myfree(pfds);
}

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	acl_int64 begin, now;
	POLL_EVENT pe;
	EVENT  *ev;

	if (__sys_poll == NULL) {
		hook_poll();
	}

	if (!acl_var_hook_sys_api) {
		return __sys_poll ? __sys_poll(fds, nfds, timeout) : -1;
	}

	ev        = fiber_io_event();
	pe.fds    = pollfd_alloc(&pe, fds, nfds);
	pe.nfds   = nfds;
	pe.fiber  = acl_fiber_running();
	pe.proc   = poll_callback;
	pe.nready = 0;

	SET_TIME(begin);
	event_poll_set(ev, &pe, timeout);

	while (1) {
		fiber_io_inc();
		acl_fiber_switch();

		if (acl_fiber_killed(pe.fiber)) {
			event_poll_clear(ev, &pe);
			acl_msg_info("%s(%d), %s: fiber-%u was killed, %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_id(pe.fiber), acl_last_serror());
			pe.nready = -1;
			break;
		}

#ifdef	USE_RING
		if (acl_ring_size(&ev->poll_list) == 0) {
#elif	defined(USE_STACK)
		if (acl_stack_size(ev->poll_list) == 0) {
#else
		if (acl_fifo_size(ev->poll_list) == 0) {
#endif
			ev->timeout = -1;
		}

		if (pe.nready != 0 || timeout == 0) {
			break;
		}

		SET_TIME(now);

		if (timeout > 0 && (now - begin >= timeout)) {
			break;
		}
	}

	pollfd_free(pe.fds);
	return pe.nready;
}
