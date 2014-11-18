/* System interfaces. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef	ACL_UNIX

#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_connect.h"

/* acl_unix_connect - connect to UNIX-domain listener */

ACL_SOCKET acl_unix_connect(const char *addr, int block_mode, int timeout)
{
#undef sun
	struct sockaddr_un sun;
	int     len = strlen(addr);
	ACL_SOCKET  sock;

	/*
	 * Translate address information to internal form.
	 */
	if (len >= (int) sizeof(sun.sun_path))
		acl_msg_fatal("unix-domain name too long: %s", addr);
	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
#ifdef HAS_SUN_LEN
	sun.sun_len = len + 1;
#endif
	memcpy(sun.sun_path, addr, len + 1);

	/*
	 * Create a client socket.
	 */
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		char tbuf[256];
		acl_msg_fatal("socket: %s", acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	/*
	 * Timed connect.
	 */
	if (timeout > 0) {
		acl_non_blocking(sock, ACL_NON_BLOCKING);
		if (acl_timed_connect(sock, (struct sockaddr *) & sun,
					sizeof(sun), timeout) < 0) {
			close(sock);
			return (-1);
		}
		if (block_mode != ACL_NON_BLOCKING)
			acl_non_blocking(sock, block_mode);
		return (sock);
	}

	/*
	 * Maybe block until connected.
	 */
	else {
		acl_non_blocking(sock, block_mode);
		if (acl_sane_connect(sock, (struct sockaddr *) & sun, sizeof(sun)) < 0
			&& acl_last_error() != EINPROGRESS) {

			close(sock);
			return (-1);
		}
		return (sock);
	}
}
#endif

