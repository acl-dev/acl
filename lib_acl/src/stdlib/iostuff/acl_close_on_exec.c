#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System interfaces. */
#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <fcntl.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"

/* Application-specific. */

#include "stdlib/acl_iostuff.h"

#define ACL_PATTERN	FD_CLOEXEC

/* close_on_exec - set/clear close-on-exec flag */

int     acl_close_on_exec(int fd, int on)
{
	int     flags;

	if ((flags = fcntl(fd, F_GETFD, 0)) < 0) {
		char tbuf[256];
		acl_msg_fatal("fcntl: get flags: %s",
			acl_last_strerror(tbuf, sizeof(tbuf)));
	}
	if (fcntl(fd, F_SETFD, on ? flags | ACL_PATTERN : flags & ~ACL_PATTERN) < 0) {
		char tbuf[256];
		acl_msg_fatal("fcntl: set close-on-exec flag %s: %s", on ? "on" : "off",
			acl_last_strerror(tbuf, sizeof(tbuf)));
	}
	return ((flags & ACL_PATTERN) != 0);
}

#endif /* ACL_UNIX */
