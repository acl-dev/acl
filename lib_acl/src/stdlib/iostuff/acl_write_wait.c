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

#if defined(ACL_HAS_POLL)

# if defined(ACL_WINDOWS)
static acl_poll_fn __sys_poll = WSAPoll;
# else
static acl_poll_fn __sys_poll = poll;
# endif

extern void set_poll4write(acl_poll_fn fn);

void set_poll4write(acl_poll_fn fn)
{
	__sys_poll = fn;
}

int acl_write_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "acl_write_wait";
	struct pollfd fds;
	int   delay = timeout * 1000;

	fds.events = POLLOUT;
	fds.revents = 0;
	fds.fd = fd;

	for (;;) {
		switch (__sys_poll(&fds, 1, delay)) {
#ifdef ACL_WINDOWS
		case SOCKET_ERROR:
#else
		case -1:
#endif
			if (acl_last_error() == ACL_EINTR)
				continue;
			acl_msg_error("%s(%d), %s: poll error(%s), fd: %d",
				__FILE__, __LINE__, myname,
				acl_last_serror(), (int) fd);
			return -1;
		case 0:
			acl_set_error(ACL_ETIMEDOUT);
			acl_msg_error("%s(%d), %s: poll return 0",
				__FILE__, __LINE__, myname);
			return -1;
		default:
			if (fds.revents & POLLOUT) {
				return 0;
			}

			if (!(fds.revents & (POLLHUP | POLLERR | POLLNVAL))) {
				acl_msg_error("%s(%d), %s: error: %s, fd: %d",
					__FILE__, __LINE__, myname,
					acl_last_serror(), fd);
				return -1;
			}

#ifdef ACL_UNIX
			acl_msg_warn("%s(%d), %s: %s, revents=%d, %d, %d, %d",
				__FILE__, __LINE__, myname,
				acl_last_serror(), fds.revents,
				fds.revents & POLLHUP,
				fds.revents& POLLERR,
				fds.revents& POLLNVAL);
#endif
			return 0;
		}
	}
}

#else

int acl_write_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "acl_write_wait";
	fd_set  wfds, xfds;
	struct timeval tv;
	struct timeval *tp;
	int  errnum;

#ifndef ACL_WINDOWS
	/*
	 * Sanity checks.
	 */
	if (FD_SETSIZE <= (unsigned) fd)
		acl_msg_fatal("%s, %s(%d): descriptor %d does not fit "
			"FD_SETSIZE %d", myname, __FILE__, __LINE__,
			(int) fd, FD_SETSIZE);
#endif

	/*
	* Guard the write() with select() so we do not depend on alarm()
	* and on signal() handlers. Restart the select when interrupted
	* by some signal. Some select() implementations may reduce the time
	* to wait when interrupted, which is exactly what we want.
	*/
	FD_ZERO(&wfds);
	FD_SET(fd, &wfds);
	FD_ZERO(&xfds);
	FD_SET(fd, &xfds);

	if (timeout >= 0) {
		tv.tv_usec = 0;
		tv.tv_sec = timeout;
		tp = &tv;
	} else
		tp = 0;

	for (;;) {
#ifdef ACL_WINDOWS
		switch (select(1, (fd_set *) 0, &wfds, &xfds, tp)) {
#else
		switch (select(fd + 1, (fd_set *) 0, &wfds, &xfds, tp)) {
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
			acl_msg_error("%s, %s(%d): select error(%s), fd(%d)",
				myname, __FILE__, __LINE__,
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
