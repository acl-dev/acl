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
#include "stdlib/acl_sys_patch.h"
#include "net/acl_connect.h"

/* acl_unix_connect - connect to UNIX-domain listener */

ACL_SOCKET acl_unix_connect(const char *addr, int block_mode, int timeout)
{
#undef sun
	struct sockaddr_un sun;
	size_t len, size;
	char  *path = sun.sun_path;
	ACL_SOCKET  sock;

	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	size = sizeof(sun.sun_path);

#ifdef ACL_LINUX
	if (*addr == '@') {
		addr++;
		size--;
		*path++ = 0;
	}
	len = strlen(addr);
#else
	len = strlen(addr);
#endif

	/* Translate address information to internal form. */
	if (len >= size || len == 0) {
		acl_msg_error("%s(%d), %s: invalid addr len=%ld, unix path=%s",
			__FILE__, __LINE__, __FUNCTION__, (long) len, addr);
		return ACL_SOCKET_INVALID;
	}

#ifdef HAS_SUN_LEN
	sun.sun_len = len + 1;
#endif
	memcpy(path, addr, len + 1);

	/* Create a client socket. */
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		acl_msg_error("%s(%d): socket: %s",
			__FUNCTION__, __LINE__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	size = sizeof(sun.sun_family) + strlen(addr) + 1;
	/* Timed connect. */
	if (timeout > 0) {
		acl_non_blocking(sock, ACL_NON_BLOCKING);
		if (acl_timed_connect(sock, (struct sockaddr *) & sun,
			(socklen_t) size, timeout) < 0) {

			acl_socket_close(sock);
			return ACL_SOCKET_INVALID;
		}
		if (block_mode != ACL_NON_BLOCKING) {
			acl_non_blocking(sock, block_mode);
		}
		return sock;
	}

	/* Maybe block until connected. */
	acl_non_blocking(sock, block_mode);
	if (acl_sane_connect(sock, (struct sockaddr *) & sun,
		(socklen_t) size) < 0 && acl_last_error() != EINPROGRESS) {

		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}
	return sock;
}
#endif
