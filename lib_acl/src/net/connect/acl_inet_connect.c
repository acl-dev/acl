#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#ifdef ACL_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#ifdef	ACL_WINDOWS
#include "stdlib/acl_mystring.h"
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_tcp_ctl.h"
#include "net/acl_netdb.h"
#include "net/acl_valid_hostname.h"
#include "net/acl_connect.h"

#endif

static int bind_local(ACL_SOCKET sock, int family, const struct addrinfo *res0)
{
	const struct addrinfo *res;

	for (res = res0; res != NULL; res = res->ai_next) {
		if (res->ai_family != family) {
			continue;
		}

#ifdef ACL_WINDOWS
		if (bind(sock, res->ai_addr, (int) res->ai_addrlen) == 0) {
#else
		if (bind(sock, res->ai_addr, res->ai_addrlen) == 0) {
#endif
			return 0;
		}
	}

	return -1;
}

/* inet_connect_one - try to connect to one address */

static ACL_SOCKET inet_connect_one(const struct addrinfo *peer,
	const struct addrinfo *local0, int blocking, int timeout)
{
	const char *myname = "inet_connect_one";
	ACL_SOCKET  sock;
	int         on;

	sock = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): create socket error: %s",
			myname, __LINE__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	/*
	acl_tcp_set_rcvbuf(sock, ACL_SOCKET_RBUF_SIZE);
	acl_tcp_set_sndbuf(sock, ACL_SOCKET_WBUF_SIZE);
	*/

	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(const void *) &on, sizeof(on)) < 0) {

		acl_msg_warn("%s(%d): setsockopt(SO_REUSEADDR): %s",
			__FILE__, __LINE__, acl_last_serror());
	}

	if (local0 != NULL && bind_local(sock, peer->ai_family, local0) < 0) {
		acl_msg_error("%s(%d): bind local error %s, fd=%d",
			myname, __LINE__, acl_last_serror(), sock);
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	/* Timed connect. */
	if (timeout > 0) {
		acl_non_blocking(sock, ACL_NON_BLOCKING);
#ifdef ACL_WINDOWS
		if (acl_timed_connect(sock, peer->ai_addr,
			(socklen_t) peer->ai_addrlen, timeout) < 0) {
#else
		if (acl_timed_connect(sock, peer->ai_addr,
			peer->ai_addrlen, timeout) < 0) {
#endif
#ifdef ACL_WINDOWS
			int err = acl_last_error();
#endif
			acl_socket_close(sock);
#ifdef ACL_WINDOWS
			acl_set_error(err);
#endif
			return ACL_SOCKET_INVALID;
		}
		if (blocking != ACL_NON_BLOCKING) {
			acl_non_blocking(sock, blocking);
		}
		return sock;
	}

	/* Maybe block until connected. */
	acl_non_blocking(sock, blocking);
#ifdef ACL_WINDOWS
	if (acl_sane_connect(sock, peer->ai_addr,
		(socklen_t) peer->ai_addrlen) < 0) {
#else
	if (acl_sane_connect(sock, peer->ai_addr, peer->ai_addrlen) < 0) {
#endif
		int  err, errnum;
		socklen_t len;

		errnum = acl_last_error();
		len = sizeof(err);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR,
			(char *) &err, &len) < 0) {
#ifdef  SUNOS5
			/*
			 * Solaris 2.4's socket emulation doesn't allow you
			 * to determine the error from a failed non-blocking
			 * connect and just returns EPIPE.  Create a fake
			 * error message for connect. -- fenner@parc.xerox.com
			 */
			if (errno == EPIPE) {
				acl_set_error(ACL_ENOTCONN);
			}
#endif
#ifdef ACL_WINDOWS
			err = acl_last_error();
#endif
			acl_socket_close(sock);
#ifdef ACL_WINDOWS
			acl_set_error(err);
#endif
			return ACL_SOCKET_INVALID;
		} else if (err != 0) {
			errnum = err;
			acl_set_error(err);
		}

#ifdef	ACL_WINDOWS
		if (errnum == ACL_EINPROGRESS || errnum == ACL_EWOULDBLOCK) {
			return sock;
		}
#elif defined(ACL_UNIX)
		if (errnum == ACL_EINPROGRESS || errnum == EISCONN) {
			return sock;
		}
#endif
		acl_socket_close(sock);
#ifdef ACL_WINDOWS
		acl_set_error(errnum);
#endif
		return ACL_SOCKET_INVALID;
	}
	return sock;
}

/* acl_inet_connect - connect to TCP listener */

ACL_SOCKET acl_inet_connect(const char *addr, int blocking, int timeout)
{
	int   h_error = 0;
	return acl_inet_connect_ex(addr, blocking, timeout, &h_error);
}

ACL_SOCKET acl_inet_connect_ex(const char *addr, int blocking,
	int timeout, int *h_error)
{
	const char *myname = "acl_inet_connect_ex";
	int err;
	ACL_SOCKET  sock;
	char  buf[256], *ptr = NULL;
	const char *peer, *local, *port;
	struct addrinfo hints, *peer_res0, *res, *local_res0;

	if (h_error) {
		*h_error = 0;
	}

	snprintf(buf, sizeof(buf) - 1, "%s", addr);
	peer = buf;
	ptr  = strchr(buf, '@');
	if (ptr != NULL) {
		*ptr  = 0;
		local = *++ptr == 0 ? NULL : ptr;
	} else {
		local = NULL;
	}

	if (acl_valid_ipv6_hostaddr(peer, 0)) {
		ptr = strrchr(peer, ACL_ADDR_SEP);
	} else if (acl_valid_ipv4_hostaddr(peer, 0)) {
		ptr = strrchr(peer, ACL_ADDR_SEP);
		if (ptr == NULL) {
			ptr = strrchr(peer, ':');
		}
	} else if (!(ptr = strrchr(buf, ACL_ADDR_SEP))) {
		ptr = strrchr(buf, ':');
	}

	if (ptr == NULL) {
		acl_msg_error("%s, %s(%d): invalid addr(%s)",
			__FILE__, myname, __LINE__, addr);
		return ACL_SOCKET_INVALID;
	}

	*ptr++ = 0;
	port   = ptr;

	if (atoi(port) <= 0) {
		acl_msg_error("%s, %s(%d): invalid port(%s)",
			__FILE__, myname, __LINE__, port);
		return ACL_SOCKET_INVALID;
	}

	if (strlen(peer) == 0) {
		acl_msg_error("%s, %s(%d): ip buf's length is 0",
			__FILE__, myname, __LINE__);
		return ACL_SOCKET_INVALID;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

#if	defined(ACL_FREEBSD)
	hints.ai_flags    = 0;
#elif	defined(ACL_MACOSX)
	hints.ai_flags    = AI_DEFAULT;
#elif	defined(ACL_ANDROID)
	hints.ai_flags    = AI_ADDRCONFIG;
#elif defined(ACL_WINDOWS)
# if _MSC_VER >= 1500
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#else
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif

	if ((err = getaddrinfo(peer, port, &hints, &peer_res0))) {
		acl_msg_error("%s(%d), %s: getaddrinfo error %s, peer=%s",
			__FILE__, __LINE__, myname, gai_strerror(err), peer);
		return ACL_SOCKET_INVALID;
	}

	if (local == NULL) {
		local_res0 = NULL;
	} else if ((err = getaddrinfo(local, "0", &hints, &local_res0))) {
		acl_msg_error("%s(%d), %s: getaddrinfo error %s, local=%s",
			__FILE__, __LINE__, myname, gai_strerror(err), local);
		return ACL_SOCKET_INVALID;
	}

	sock = ACL_SOCKET_INVALID;

	for (res = peer_res0; res != NULL ; res = res->ai_next) {
		sock = inet_connect_one(res, local_res0, blocking, timeout);
		if (sock != ACL_SOCKET_INVALID) {
			break;
		}
	}

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d) %s: connect error %s, addr=%s",
			__FILE__, __LINE__, myname, acl_last_serror(), addr);
	}

	if (peer_res0) {
		freeaddrinfo(peer_res0);
	}
	if (local_res0) {
		freeaddrinfo(local_res0);
	}

	return sock;
}
