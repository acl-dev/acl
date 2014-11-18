/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_sane_socketpair.h"

/* sane_socketpair - sanitize socketpair() error returns */

int acl_sane_socketpair(int domain, int type, int protocol, int result[2])
{
	static int socketpair_ok_errors[] = {
		EINTR,
		0,
	};
	int     count;
	int     err;
	int     ret;

	/*
	 * Solaris socketpair() can fail with EINTR.
	 */
	while ((ret = socketpair(domain, type, protocol, result)) < 0) {
		for (count = 0; /* void */ ; count++) {
			if ((err = socketpair_ok_errors[count]) == 0)
				return (ret);
			if (acl_last_error() == err) {
				char tbuf[256];
				acl_msg_warn("socketpair: %s (trying again)",
					acl_last_strerror(tbuf, sizeof(tbuf)));
				sleep(1);
				break;
			}
		}
	}
	return (ret);
}
#endif /* ACL_UNIX*/

