/* System interfaces. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <stdio.h>

/* Utility library. */

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_sane_inet.h"
#include "net/acl_listen.h"

/* acl_unix_listen - create UNIX-domain listener */

ACL_SOCKET acl_unix_listen(const char *addr, int backlog, unsigned flag)
{
#undef sun
	struct sockaddr_un sun;
	size_t len, size;
	char  *path = sun.sun_path;
	ACL_SOCKET sock;

	memset((char *) &sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	size = sizeof(sun.sun_path);

#ifdef	ACL_LINUX
	/* for Linux abstract unix path, we should skip first '@' which was
	 * marked astract unix in the first of the path by acl.
	 */
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

	/* Create a listener socket. Do whatever we can so we don't run into
	 * trouble when this process is restarted after crash.
	 */
	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: socket: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	/* if path is same as sun.sun_path, that's to say the path is not the
	 * Linux abstract unix
	 */
	if (path == sun.sun_path) {
		(void) unlink(addr);
	}

	size= (sizeof(sun.sun_family) + strlen(addr)) + 1;

	if (bind(sock, (struct sockaddr *) & sun, (socklen_t) size) < 0) {
		acl_msg_error("%s(%d), %s: bind: %s: %s", __FILE__, __LINE__,
			__FUNCTION__, addr, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}
#ifdef FCHMOD_UNIX_SOCKETS
	if (path == sun.sun_path && fchmod(sock, 0666) < 0) {
		acl_msg_warn("%s(%d), %s: fchmod socket %s: %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
	}
#else
	if (path == sun.sun_path && chmod(addr, 0666) < 0) {
		acl_msg_warn("%s(%d), %s: chmod socket %s: %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
	}
#endif
	acl_non_blocking(sock, flag & ACL_INET_FLAG_NBLOCK ?
		ACL_NON_BLOCKING : ACL_BLOCKING);

	if (listen(sock, backlog) < 0) {
		acl_msg_error("%s(%d), %s: listen %s error %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	return sock;
}

/* acl_unix_accept - accept connection */

ACL_SOCKET acl_unix_accept(ACL_SOCKET fd)
{
	return acl_sane_accept(fd, (struct sockaddr *) 0, (socklen_t *) 0);
}

#endif
