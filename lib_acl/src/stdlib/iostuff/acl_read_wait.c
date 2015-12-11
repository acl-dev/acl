#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <string.h>  /* in SUNOS, FD_ZERO need memset() */
#include <errno.h>

#endif

#ifdef	ACL_UNIX
#include <fcntl.h>
#include <sys/poll.h>   
#include <unistd.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "../../init/init.h"

#if	defined(LINUX2) && !defined(MINGW)

#include "init/acl_init.h"
#include "thread/acl_pthread.h"
#include <sys/epoll.h>

static int *main_epoll_read_fd = NULL;

static void main_epoll_end(void)
{
	if (main_epoll_read_fd != NULL) {
		close(*main_epoll_read_fd);
		acl_myfree(main_epoll_read_fd);
	}
}

static acl_pthread_key_t epoll_key;
static acl_pthread_once_t epoll_once = ACL_PTHREAD_ONCE_INIT;

static void thread_epoll_end(void *buf)
{
	int *epoll_fd = (int*) buf;

	close(*epoll_fd);
	acl_myfree(epoll_fd);
}

static void thread_epoll_init(void)
{
	acl_assert(acl_pthread_key_create(&epoll_key, thread_epoll_end) == 0);
}

int acl_read_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "acl_read_wait";
	int op = EPOLL_CTL_ADD, delay = timeout * 1000, *epoll_fd;
	struct epoll_event ee, events[1];

	acl_assert(acl_pthread_once(&epoll_once, thread_epoll_init) == 0);
	epoll_fd = (int*) acl_pthread_getspecific(epoll_key);
	if (epoll_fd == NULL) {
		epoll_fd = (int*) acl_mymalloc(sizeof(int));
		acl_assert(acl_pthread_setspecific(epoll_key, epoll_fd) == 0);
		if ((unsigned long) acl_pthread_self()
			== acl_main_thread_self())
		{
			main_epoll_read_fd = epoll_fd;
			atexit(main_epoll_end);
		}

		*epoll_fd = epoll_create(1);
	}

	ee.events = EPOLLIN | EPOLLHUP | EPOLLERR;
	ee.data.u64 = 0;
	ee.data.fd = fd;
	if (epoll_ctl(*epoll_fd, op, fd, &ee) == -1) {
		acl_msg_error("%s(%d): epoll_ctl error: %s, fd: %d",
			myname, __LINE__, acl_last_serror(), fd);
		return -1;
	}

	if (epoll_wait(*epoll_fd, events, 1, delay) == -1) {
		acl_msg_error("%s(%d): epoll_wait error: %s, fd: %d",
			myname, __LINE__, acl_last_serror(), fd);
		return -1;
	}

	ee.events = 0;
	ee.data.u64 = 0;
	ee.data.fd = fd;
	if (epoll_ctl(*epoll_fd, EPOLL_CTL_DEL, fd, &ee) == -1) {
		acl_msg_error("%s(%d): epoll_ctl error: %s, fd: %d",
			myname, __LINE__, acl_last_serror(), fd);
		return -1;
	}

	if ((events[0].events & (EPOLLERR | EPOLLHUP)) != 0)
		return -1;

	if ((events[0].events & EPOLLIN) == 0) {
		acl_set_error(ACL_ETIMEDOUT);
		return -1;
	}

	return 0;
}

#elif	defined(ACL_UNIX)

int acl_read_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "acl_read_wait";
	struct pollfd fds;
	int   delay = timeout * 1000;

	fds.events = POLLIN | POLLHUP | POLLERR;
	fds.fd = fd;

	acl_set_error(0);

	for (;;) {
		switch (poll(&fds, 1, delay)) {
		case -1:
			if (acl_last_error() == ACL_EINTR)
				continue;

			acl_msg_error("%s(%d), %s: poll error(%s), fd: %d",
				__FILE__, __LINE__, myname,
				acl_last_serror(), (int) fd);
			return -1;
		case 0:
			acl_set_error(ACL_ETIMEDOUT);
			return -1;
		default:
			if (fds.revents & (POLLHUP | POLLERR))
				return -1;
			else if ((fds.revents & POLLIN))
				return 0;
			else
				return -1;
		}
	}
}

#elif defined(WIN32_XX)

static HANDLE __handle = NULL;

int acl_read_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "acl_read_wait";
	OVERLAPPED *overlapped, *lpOverlapped;
	DWORD recvBytes;
	BOOL isSuccess;
	ACL_SOCKET fd2;
	DWORD bytesTransferred = 0;
	DWORD delay = timeout * 1000;
	HANDLE h2;

	if (__handle == NULL)
		__handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE,
				NULL, 0, 0);
	if (__handle == NULL) {
		acl_msg_error("CreateIoCompletionPort: %s",
			acl_last_serror());
		return -1;
	}

	overlapped = acl_mycalloc(1, sizeof(OVERLAPPED));

	h2  = CreateIoCompletionPort((HANDLE) fd, __handle,
		(DWORD) overlapped, 0);
	if (h2 == NULL)
		acl_myfree(overlapped);
	else if (h2 != __handle) {
		acl_msg_error("invalid h2 by CreateIoCompletionPort: %s",
			acl_last_serror());
		CloseHandle(h2);
		return -1;
	}

	if (ReadFile((HANDLE) fd, NULL, 0, &recvBytes, overlapped) == FALSE
		&& acl_last_error() != ERROR_IO_PENDING)
	{
		acl_msg_warn("%s(%d): ReadFile error(%s)",
			myname, __LINE__, acl_last_serror());
		return -1;
	}

	delay = 6000;

	while (1)
	{
		isSuccess = GetQueuedCompletionStatus(__handle,
				&bytesTransferred,
				(DWORD*) &fd2,
				(OVERLAPPED**) &lpOverlapped,
				delay);
		if (lpOverlapped == NULL)
			break;

		if (HasOverlappedIoCompleted(lpOverlapped))
			acl_myfree(lpOverlapped);

		if (isSuccess)
			return 0;
	}

	acl_msg_warn("timeout error: %s", acl_last_serror());
	return -1;
}

#else

int acl_read_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "acl_read_wait";
	fd_set  rfds, xfds;
	struct timeval tv;
	struct timeval *tp;
	int  errnum;

	/*
	 * Sanity checks.
	 */
#ifndef ACL_WINDOWS
	if (FD_SETSIZE <= (unsigned) fd)
		acl_msg_fatal("%s(%d), %s: descriptor %d does not fit "
			"FD_SETSIZE %d", __FILE__, __LINE__, myname,
			(int) fd, FD_SETSIZE);
#endif

	/*
	 * Guard the write() with select() so we do not depend on alarm()
	 * and on signal() handlers. Restart the select when interrupted
	 * by some signal. Some select() implementations may reduce the
	 * time to wait when interrupted, which is exactly what we want.
	 */
	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	FD_ZERO(&xfds);
	FD_SET(fd, &xfds);

	if (timeout >= 0) {
		tv.tv_usec = 0;
		tv.tv_sec = timeout;
		tp = &tv;
	} else
		tp = 0;

	acl_set_error(0);

	for (;;) {
#ifdef ACL_WINDOWS
		switch (select(1, &rfds, (fd_set *) 0, &xfds, tp)) {
#else
		switch (select(fd + 1, &rfds, (fd_set *) 0, &xfds, tp)) {
#endif
		case -1:
			errnum = acl_last_error();
#ifdef	ACL_WINDOWS
			if (errnum == WSAEINPROGRESS
				|| errnum == WSAEWOULDBLOCK
				|| errnum == ACL_EINTR)
			{
				continue;
			}
#else
			if (errnum == ACL_EINTR)
				continue;
#endif
			acl_msg_error("%s(%d), %s: select error(%s), fd: %d",
				__FILE__, __LINE__, myname,
				acl_last_serror(), (int) fd);
			return -1;
		case 0:
			acl_set_error(ACL_ETIMEDOUT);
			return -1;
		default:
			return 0;
		}
	}
}

#endif
