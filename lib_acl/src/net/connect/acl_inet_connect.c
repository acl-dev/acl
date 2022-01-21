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

/* connect_one - try to connect to one address */

static ACL_SOCKET connect_one(const struct addrinfo *peer,
	const struct addrinfo *local0, int blocking, int timeout)
{
	ACL_SOCKET  sock;
	int         on;

	sock = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): create socket error: %s",
			__FUNCTION__, __LINE__, acl_last_serror());
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
			__FUNCTION__, __LINE__, acl_last_serror(), sock);
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	/* Timed connect. */
	if (timeout > 0) {
		acl_non_blocking(sock, ACL_NON_BLOCKING);
#ifdef ACL_WINDOWS
		if (acl_timed_connect_ms(sock, peer->ai_addr,
			(socklen_t) peer->ai_addrlen, timeout) < 0) {
#else
		if (acl_timed_connect_ms(sock, peer->ai_addr,
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
	return acl_inet_timed_connect(addr, blocking, timeout * 1000, h_error);
}

static struct addrinfo *try_numeric_addr(int family, const char *name,
	const char *service, struct addrinfo *ai_buf, ACL_SOCKADDR *in_buf)
{
	size_t addrlen;

	if (family != PF_INET && family != PF_INET6) {
		if (acl_valid_ipv6_hostaddr(name, 0)) {
			family = AF_INET6;
		} else if (acl_valid_ipv4_hostaddr(name, 0)) {
			family = PF_INET;
		} else {
			return NULL;
		}
	}

	addrlen = acl_inet_pton(family, name, (struct sockaddr*) in_buf);
	if (addrlen == 0) {
		acl_msg_warn("%s(%d), %s: acl_inet_pton error, name=%s",
			__FILE__, __LINE__, __FUNCTION__, name);
		return NULL;
	}

	if (family == PF_INET) {
		in_buf->in.sin_port   = htons(atoi(service));
	} else {
		in_buf->in6.sin6_port = htons(atoi(service));
	}

	ai_buf->ai_flags    = AI_NUMERICHOST;
	ai_buf->ai_family   = family;
	ai_buf->ai_socktype = SOCK_STREAM;
	ai_buf->ai_protocol = 0;
	ai_buf->ai_addrlen  = addrlen;
	ai_buf->ai_addr     = (struct sockaddr*) in_buf;
	return ai_buf;
}

static struct addrinfo *resolve_addr(const char *name, const char *service)
{
	struct addrinfo hints, *res0;
	int err;

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

	if ((err = getaddrinfo(name, service, &hints, &res0)) == 0) {
		return res0;
	}

	acl_msg_error("%s(%d), %s: getaddrinfo error %s, peer=%s",
		__FILE__, __LINE__, __FUNCTION__, gai_strerror(err), name);
	return NULL;
}

struct addr_res {
	int peer_family;
	char buf[2 * sizeof(ACL_SOCKADDR) + 128];
	ACL_SOCKADDR peer_in;
	ACL_SOCKADDR local_in;
	struct addrinfo  peer_buf;
	struct addrinfo  local_buf;
	struct addrinfo *peer_res0;
	struct addrinfo *local_res0;
	const char *peer_port;
};

static int parse_addr(const char *addr, struct addr_res *res)
{
	char *ptr;
	const char *peer, *local;

	res->peer_family = PF_UNSPEC;

	snprintf(res->buf, sizeof(res->buf) - 1, "%s", addr);
	peer = res->buf;
	ptr  = strchr(res->buf, '@');
	if (ptr != NULL) {
		*ptr  = 0;
		local = *++ptr == 0 ? NULL : ptr;
	} else {
		local = NULL;
	}

	if (acl_valid_ipv6_hostaddr(peer, 0)) {
		ptr = strrchr(peer, ACL_ADDR_SEP);
		res->peer_family = PF_INET6;
	} else if (acl_valid_ipv4_hostaddr(peer, 0)) {
		ptr = strrchr(peer, ACL_ADDR_SEP);
		if (ptr == NULL) {
			ptr = strrchr(peer, ':');
		}
		res->peer_family = PF_INET;
	} else if (!(ptr = strrchr(res->buf, ACL_ADDR_SEP))) {
		ptr = strrchr(res->buf, ':');
	}

	if (ptr == NULL) {
		acl_msg_error("%s, %s(%d): invalid addr(%s)",
			__FILE__, __FUNCTION__, __LINE__, addr);
		return -1;
	}

	*ptr++ = 0;
	res->peer_port = ptr;

	if (atoi(res->peer_port) <= 0) {
		acl_msg_error("%s, %s(%d): invalid port(%s)",
			__FILE__, __FUNCTION__, __LINE__, res->peer_port);
		return -1;
	}

	if (strlen(peer) == 0) {
		acl_msg_error("%s, %s(%d): ip buf's length is 0",
			__FILE__, __FUNCTION__, __LINE__);
		return -1;
	}

	res->peer_res0 = try_numeric_addr(res->peer_family, peer,
		res->peer_port, &res->peer_buf, &res->peer_in);
	if (res->peer_res0 == NULL && (res->peer_res0 =
		 resolve_addr(peer, res->peer_port)) == NULL) {
		return -1;
	}

	if (local == NULL) {
		res->local_res0 = NULL;
	} else if ((res->local_res0 = try_numeric_addr(PF_UNSPEC, local, "0",
		 &res->local_buf, &res->local_in)) == NULL) {
		res->local_res0 = resolve_addr(local, "0");
	}

	return 0;
}

ACL_SOCKET acl_inet_timed_connect(const char *addr, int blocking,
	int timeout, int *h_error)
{
	ACL_SOCKET  sock;
	struct addr_res ares;
	struct addrinfo *res;

	if (h_error) {
		*h_error = 0;
	}

	/* we should fill the ares with 0 to init all members int it. */
	memset(&ares, 0, sizeof(ares));

	if (parse_addr(addr, &ares) == -1) {
		return ACL_SOCKET_INVALID;
	}

	sock = ACL_SOCKET_INVALID;

	for (res = ares.peer_res0; res != NULL ; res = res->ai_next) {
		sock = connect_one(res, ares.local_res0, blocking, timeout);
		if (sock != ACL_SOCKET_INVALID) {
			break;
		}
	}

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d) %s: connect %s error %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
	}

	if (ares.peer_res0 != &ares.peer_buf) {
		freeaddrinfo(ares.peer_res0);
	}

	if (ares.local_res0 && ares.local_res0 != &ares.local_buf) {
		freeaddrinfo(ares.local_res0);
	}

	return sock;
}
