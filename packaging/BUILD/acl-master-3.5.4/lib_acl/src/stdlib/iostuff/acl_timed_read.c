
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
# include <unistd.h>
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_iostuff.h"

/* acl_timed_read - read with deadline */

int acl_timed_read(ACL_SOCKET fd, void *buf, unsigned len,
	int timeout, void *context acl_unused)
{
	int     ret;

	/*
	 * Wait for a limited amount of time for something to happen.
	 * If nothing happens, report an ETIMEDOUT error.
	 * 
	 * XXX Solaris 8 read() fails with EAGAIN after read-select()
	 * returns success.
	 */
	for (;;) {
		if (timeout > 0 && acl_read_wait(fd, timeout) < 0)
			return -1;
		ret = acl_socket_read(fd, buf, len, 0, NULL, NULL);
		if (ret < 0 && timeout > 0 && acl_last_error() == ACL_EAGAIN)
		{
			acl_msg_warn("read() returns EAGAIN on"
				" a readable file descriptor!");
			acl_msg_warn("pausing to avoid going into"
				" a tight select/read loop!");
			sleep(1);
		} else
			return ret;
	}
}
