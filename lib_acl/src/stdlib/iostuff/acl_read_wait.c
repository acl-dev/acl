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

static int select_read_wait(ACL_SOCKET fd, int timeout)
{
	fd_set  read_fds;
	fd_set  except_fds;
	struct timeval tv;
	struct timeval *tp;
#ifdef	WIN32
	int  errnum;
#endif

#ifdef	ACL_UNIX
	/*
	* Sanity checks.
	*/
	if (FD_SETSIZE <= (unsigned) fd)
		acl_msg_panic("descriptor %d does not fit FD_SETSIZE %d",
		fd, FD_SETSIZE);
#endif

	/*
	* Guard the write() with select() so we do not depend on alarm() and on
	* signal() handlers. Restart the select when interrupted by some signal.
	* Some select() implementations may reduce the time to wait when
	* interrupted, which is exactly what we want.
	*/
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	FD_ZERO(&except_fds);
	FD_SET(fd, &except_fds);
	if (timeout >= 0) {
		tv.tv_usec = 0;
		tv.tv_sec = timeout;
		tp = &tv;
	} else {
		tp = 0;
	}

	for (;;) {
		switch (select(fd + 1, (fd_set *) &read_fds, (fd_set *) 0, &except_fds, tp)) {
		case -1:
#ifdef	WIN32
			errnum = WSAGetLastError();
			if (errnum != WSAEINPROGRESS && errnum != WSAEWOULDBLOCK) {
				acl_msg_error("select error");
				return (-1);
			}
#else
			if (acl_last_error() != ACL_EINTR) {
				char tbuf[256];
				acl_msg_error("%s(%d): select error(%s)",
					__FILE__, __LINE__,
					acl_last_strerror(tbuf, sizeof(tbuf)));
				return (-1);
			}
#endif
			continue;
		case 0:
			acl_set_error(ACL_ETIMEDOUT);
			return (-1);
		default:
			return (0);
		}
	}
}

#ifdef ACL_UNIX

#include <stdio.h>
static int poll_read_wait(ACL_SOCKET fd, int timeout)
{
	const char *myname = "poll_read_wait";
	struct pollfd fds;
	int   delay = timeout * 1000;

	fds.events = POLLIN | POLLHUP | POLLERR;
	fds.fd = fd;

	for (;;) {
		switch (poll(&fds, 1, delay)) {
		case -1:
			if (acl_last_error() != ACL_EINTR) {
				char tbuf[256];
				acl_msg_error("%s: poll error(%s)", myname,
					acl_last_strerror(tbuf, sizeof(tbuf)));
				return (-1);
			}
			continue;
		case 0:
			acl_set_error(ACL_ETIMEDOUT);
			return (-1);
		default:
			if ((fds.revents & (POLLHUP | POLLERR))
				|| !(fds.revents & POLLIN))
			{
				return (-1);
			}
			return (0);
		}
	}
}
#endif

int acl_read_wait(ACL_SOCKET fd, int timeout)
{
#ifdef	ACL_UNIX
	if (__acl_var_use_poll)
		return (poll_read_wait(fd, timeout));
#endif

	return (select_read_wait(fd, timeout));
}
