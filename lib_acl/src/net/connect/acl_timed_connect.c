#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef ACL_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_connect.h"

#endif

int acl_timed_connect(ACL_SOCKET sock, const struct sockaddr * sa,
	socklen_t len, int timeout)
{
	int   err;

	/*
	 * Sanity check. Just like with timed_wait(), the timeout must be a
	 * positive number.
	 */
	if (timeout < 0) {
		acl_msg_panic("timed_connect: bad timeout: %d", timeout);
	}

	/*
	 * Start the connection, and handle all possible results.
	 */
	if (acl_sane_connect(sock, sa, len) == 0) {
		return 0;
	}

	errno = acl_last_error();

#ifdef	ACL_UNIX
	if (errno != ACL_EINPROGRESS) {
		return -1;
	}
#elif defined(ACL_WINDOWS)
	if (errno != ACL_EINPROGRESS && errno != ACL_EWOULDBLOCK) {
		return -1;
	}
#endif

	/*
	 * A connection is in progress. Wait for a limited amount of time for
	 * something to happen. If nothing happens, report an error.
	 */
	if (acl_write_wait(sock, timeout) < 0) {
		return -1;
	}

	/*
	 * Something happened. Some Solaris 2 versions have getsockopt() itself
	 * return the error, instead of returning it via the parameter list.
	 */
	len = sizeof(err);
	if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len) < 0) {
#ifdef  SUNOS5
	/*
	 * Solaris 2.4's socket emulation doesn't allow you
	 * to determine the error from a failed non-blocking
	 * connect and just returns EPIPE.  Create a fake
	 * error message for connect.   -- fenner@parc.xerox.com
	 */
		if (errno == EPIPE) {
			acl_set_error(ACL_ENOTCONN);
		}
#endif
		return -1;
	}

	if (err != 0) {
		acl_set_error(err);
		return -1;
	} else {
		return 0;
	}
}
