/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include <errno.h>

#endif

#ifdef ACL_UNIX
# include <sys/time.h>
#endif
#include <string.h>

#ifdef USE_SYS_SELECT_H
#include <sys/select.h>
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"

/* acl_readable - see if file descriptor is readable */

int acl_readable(ACL_SOCKET fd)
{
	struct timeval tv;
	fd_set  read_fds;
	fd_set  except_fds;

	/*
	 * Sanity checks.
	 */
	if ((unsigned) fd >= FD_SETSIZE)
		acl_msg_fatal("fd %d does not fit in FD_SETSIZE", fd);

	/*
	 * Initialize.
	 */
	FD_ZERO(&read_fds);
	FD_SET(fd, &read_fds);
	FD_ZERO(&except_fds);
	FD_SET(fd, &except_fds);
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	/*
	 * Loop until we have an authoritative answer.
	 */
	for (;;) {
		switch (select(fd + 1, &read_fds,
				(fd_set *) 0, &except_fds, &tv)) {
		case -1:
			if (acl_last_error() != ACL_EINTR) {
				char tbuf[256];
				acl_msg_fatal("select: %s", acl_last_strerror(tbuf, sizeof(tbuf)));
			}
			continue;
		default:
			return (FD_ISSET(fd, &read_fds));
		case 0:
			return (0);
		}
	}
}

