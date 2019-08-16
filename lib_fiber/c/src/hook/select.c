#include "stdafx.h"
#ifndef FD_SETSIZE
#define FD_SETSIZE 10240
#endif
#include "common.h"

#include "event.h"
#include "fiber.h"

#ifdef SYS_WIN
typedef int (WINAPI *select_fn)(int, fd_set *, fd_set *,
	fd_set *, const struct timeval *);
#else
typedef int (*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);
#endif

static select_fn __sys_select = NULL;


static void hook_api(void)
{
#ifdef SYS_UNIX
	__sys_select = (select_fn) dlsym(RTLD_NEXT, "select");
	assert(__sys_select);
#else
	__sys_select = select;
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

#ifdef SYS_WIN
static struct pollfd *get_pollfd(struct pollfd fds[], int cnt, socket_t fd)
{
	int i;

	for (i = 0; i < cnt; i++) {
		if (fds[i].fd = fd) {
			return &fds[i];
		}
	}

	return NULL;
}

static int set_fdset(struct pollfd fds[], unsigned nfds, unsigned *cnt,
	fd_set *rset, int oper)
{
	unsigned int i;
	struct pollfd *pfd;

	for (i = 0; i < rset->fd_count; i++) {
		pfd = get_pollfd(fds, *cnt, rset->fd_array[i]);
		if (pfd) {
			pfd->events |= oper;
		} else if (*cnt >= nfds) {
			msg_error("%s: overflow, nfds=%d, cnt=%d, fd=%u",
				__FUNCTION__, nfds, *cnt, rset->fd_array[i]);
			return -1;
		} else {
			fds[i].events  = oper;
			fds[i].fd      = rset->fd_array[i];
			fds[i].revents = 0;
			(*cnt)++;
		}
	}
	return 0;
}

static struct pollfd *pfds_create(int *nfds, fd_set *readfds,
	fd_set *writefds, fd_set *exceptfds)
{
	struct pollfd *fds;
	unsigned cnt = 0;

	*nfds = 0;
	if (readfds && (int) readfds->fd_count > *nfds) {
		*nfds = readfds->fd_count;
	}
	if (writefds && (int) writefds->fd_count > *nfds) {
		*nfds = writefds->fd_count;
	}
	if (exceptfds && (int) exceptfds->fd_count > *nfds) {
		*nfds = exceptfds->fd_count;
	}

	fds = (struct pollfd *) mem_calloc(*nfds + 1, sizeof(struct pollfd));
	if (readfds && set_fdset(fds, *nfds, &cnt, readfds, POLLIN) == -1) {
		mem_free(fds);
		return NULL;
	}
	if (writefds && set_fdset(fds, *nfds, &cnt, writefds, POLLOUT) == -1) {
		mem_free(fds);
		return NULL;
	}
	if (exceptfds && set_fdset(fds, *nfds, &cnt, exceptfds, POLLERR) == -1) {
		mem_free(fds);
		return NULL;
	}

	return fds;
}
#else
static struct pollfd *pfds_create(int *nfds, fd_set *readfds,
	fd_set *writefds, fd_set *exceptfds fiber_unused)
{
	int fd;
	struct pollfd *fds;

	fds = (struct pollfd *) mem_calloc(*nfds + 1, sizeof(struct pollfd));

	for (fd = 0; fd < *nfds; fd++) {
		if (readfds && FD_ISSET(fd, readfds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLIN;
		}

		if (writefds && FD_ISSET(fd, writefds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLOUT;
		}

#if 0
		if (exceptfds && FD_ISSET(fd, exceptfds)) {
			fds[fd].fd = fd;
			fds[fd].events |= POLLERR | POLLHUP;
		}
#endif
	}
	return fds;
}

#endif

#ifdef SYS_WIN
int WINAPI acl_fiber_select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, const struct timeval *timeout)
#else
int acl_fiber_select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout)
#endif
{
	socket_t fd;
	struct pollfd *fds;
	int i, timo, n, nready = 0;

	if (__sys_select == NULL)
		hook_init();

	if (!var_hook_sys_api)
		return __sys_select ? __sys_select
			(nfds, readfds, writefds, exceptfds, timeout) : -1;

	fds = pfds_create(&nfds, readfds, writefds, exceptfds);
	if (fds == NULL) {
		fiber_save_errno(FIBER_EINVAL);
		return -1;
	}

	if (timeout != NULL)
		timo = (int) (timeout->tv_sec * 1000 + timeout->tv_usec / 1000);
	else
		timo = -1;

	n = acl_fiber_poll(fds, nfds, timo);

	if (readfds)
		FD_ZERO(readfds);
	if (writefds)
		FD_ZERO(writefds);
	if (exceptfds)
		FD_ZERO(exceptfds);

	for (i = 0; i < nfds && nready < n; i++) {
		if ((fd = fds[i].fd) == INVALID_SOCKET) {
			continue;
		}

		if (readfds && (fds[i].revents & POLLIN)) {
			FD_SET(fd, readfds);
			nready++;
		}

		if (writefds && (fds[i].revents & POLLOUT)) {
			FD_SET(fd, writefds);
			nready++;
		}

		if (exceptfds && (fds[i].revents & (POLLERR | POLLHUP))) {
			FD_SET(fd, exceptfds);
			nready++;
		}
	}

	mem_free(fds);
	return nready;
}

#ifdef SYS_UNIX
int select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout)
{
	return acl_fiber_select(nfds, readfds, writefds, exceptfds, timeout);
}
#endif
