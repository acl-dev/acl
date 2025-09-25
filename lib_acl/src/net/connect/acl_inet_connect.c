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
#include "net/acl_sane_socket.h"
#include "net/acl_connect.h"

#endif

struct addr_res {
    int peer_family;
    char buf[2 * sizeof(ACL_SOCKADDR) + 128];
    ACL_SOCKADDR peer_in;
    ACL_SOCKADDR local_in;
    struct addrinfo  peer_buf;
    struct addrinfo  local_buf;
    struct addrinfo *peer_res0;
    struct addrinfo *local_res0;
    const char *iface;
    const char *peer_port;
};

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
	const struct addr_res *local, int blocking, int timeout, unsigned *flags)
{
	int         on;
	struct addrinfo *local0 = local->local_res0;
	ACL_SOCKET sock = socket(peer->ai_family, peer->ai_socktype,
				peer->ai_protocol);

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): create socket error: %s",
			__FUNCTION__, __LINE__, acl_last_serror());
		if (flags) {
			*flags |= ACL_CONNECT_F_CREATE_SOCKET_ERR;
		}
		return ACL_SOCKET_INVALID;
	}

	/*
	acl_tcp_set_rcvbuf(sock, ACL_SOCKET_RBUF_SIZE);
	acl_tcp_set_sndbuf(sock, ACL_SOCKET_WBUF_SIZE);
	*/

	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on)) < 0) {
		if (flags) {
			*flags |= ACL_CONNECT_F_REUSE_ADDR_ERR;
		}
		acl_msg_warn("%s(%d): setsockopt(SO_REUSEADDR): %s",
			__FILE__, __LINE__, acl_last_serror());
	}

	/* Check and try to bind the local IP address. */
	if (local0 != NULL) {
		if (bind_local(sock, peer->ai_family, local0) < 0) {
			if (flags) {
				*flags |= ACL_CONNECT_F_BIND_IP_ERR;
			}
#if defined(CHECK_BIND_LOCAL_ERROR)
			acl_msg_error("%s(%d): bind local error %s, fd=%d",
				__FUNCTION__, __LINE__, acl_last_serror(), sock);
			acl_socket_close(sock);
			return ACL_SOCKET_INVALID;
#endif
		} else if (flags) {
			*flags |= ACL_CONNECT_F_BIND_IP_OK;
		}
	}
	/* Check and try bind the local network interface. */
	else if (local->iface != NULL) {
		if (acl_bind_interface(sock, local->iface) == -1) {
			if (flags) {
				*flags |= ACL_CONNECT_F_BIND_IFACE_ERR;
			}
#if defined(CHECK_BIND_LOCAL_ERROR)
			acl_msg_error("%s(%d): bind interface=%s error=%s",
				__FUNCTION__, __LINE__, local->iface,
				acl_last_serror());
			acl_socket_close(sock);
			return ACL_SOCKET_INVALID;
#endif
		} else if (flags) {
			*flags |= ACL_CONNECT_F_BIND_IFACE_OK;
		}
	}

	/* Timed connect. */
	if (timeout > 0) {
		acl_non_blocking(sock, ACL_NON_BLOCKING);
#ifdef ACL_WINDOWS
		if (acl_timed_connect_ms(sock, peer->ai_addr,
			(socklen_t) peer->ai_addrlen, timeout) < 0) {
#else
		if (acl_timed_connect_ms2(sock, peer->ai_addr,
			peer->ai_addrlen, timeout, flags) < 0) {
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
		int  err;
		socklen_t len;
		int errnum = acl_last_error();

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
		}
		if (err != 0) {
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
	unsigned flags = 0;
	return acl_inet_connect2(addr, blocking, timeout, &flags);
}

ACL_SOCKET acl_inet_connect2(const char *addr, int blocking,
	int timeout, unsigned *flags)
{
	return acl_inet_timed_connect(addr, blocking, timeout * 1000, flags);
}

static struct addrinfo *try_numeric_addr(int family, const char *name,
	const char *service, struct addrinfo *ai_buf, ACL_SOCKADDR *in_buf)
{
	size_t addrlen;
	int port = acl_safe_atoi(service, -1);

	if (port <= 0 || port > 65535) {
		acl_msg_warn("%s(%d), %s: port=%s invalid",
			__FILE__, __LINE__, __FUNCTION__, service);
		return NULL;
	}

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
		in_buf->in.sin_port   = htons(port);
	} else {
		in_buf->in6.sin6_port = htons(port);
	}

	ai_buf->ai_flags    = AI_NUMERICHOST;
	ai_buf->ai_family   = family;
	ai_buf->ai_socktype = SOCK_STREAM;
	ai_buf->ai_protocol = 0;
	ai_buf->ai_addrlen  = (socklen_t) addrlen;
	ai_buf->ai_addr     = (struct sockaddr*) in_buf;
	return ai_buf;
}

static struct addrinfo *resolve_addr(const char *name, const char *service)
{
	struct addrinfo hints, *res0 = NULL;
	int err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

#if	defined(ACL_FREEBSD)
	hints.ai_flags    = 0;
#elif	defined(ACL_MACOSX)
	hints.ai_flags    = AI_DEFAULT;
#elif	defined(ACL_OHOS)
	hints.ai_flags    = 0;
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

	acl_msg_error("%s(%d), %s: getaddrinfo error(%d) %s, peer=%s",
		__FILE__, __LINE__, __FUNCTION__, err, gai_strerror(err), name);
	return NULL;
}

static int parse_addr(const char *addr, struct addr_res *res)
{
	char *ptr, *local;
	const char *peer;

	res->peer_family = PF_UNSPEC;

	snprintf(res->buf, sizeof(res->buf) - 1, "%s", addr);
	peer = res->buf;

	/* @local_ip or #local_interface */

	if ((local = strchr(res->buf, '@')) != NULL) {
		*local = 0;
		if (*++local == 0) {
			local = NULL;
		}
	}

	if (local == NULL && (ptr = strchr(res->buf, '#')) != NULL) {
		*ptr = 0;
		if (*++ptr != 0) {
			res->iface = ptr;
		}
	}

	if (acl_valid_ipv6_hostaddr(peer, 0)) {
		if (*peer == '[') {
			char *p1;
			++peer;
			p1 = strchr(peer, ']');
			if (p1 == NULL) {
				acl_msg_error("%s, %s(%d): no ']' in addr(%s)",
					__FILE__, __FUNCTION__, __LINE__, addr);
				return -1;
			}
			*p1++ = 0;
			ptr = strchr(p1, ':');
			if (ptr == NULL) {
				ptr = strchr(p1, ACL_ADDR_SEP);
			}
		} else {
			ptr = strrchr(peer, ACL_ADDR_SEP);
		}
		res->peer_family = PF_INET6;
	} else if (acl_valid_ipv4_hostaddr(peer, 0)) {
		ptr = strrchr(peer, ACL_ADDR_SEP);
		if (ptr == NULL) {
			ptr = strrchr(peer, ':');
		}
		res->peer_family = PF_INET;
	} else if ((ptr = strrchr(res->buf, ACL_ADDR_SEP)) == NULL) {
		ptr = strrchr(res->buf, ':');
	}

	if (ptr == NULL) {
		acl_msg_error("%s, %s(%d): invalid addr(%s)",
			__FILE__, __FUNCTION__, __LINE__, addr);
		return -1;
	}

	*ptr++ = 0;
	res->peer_port = ptr;

	if (acl_safe_atoi(res->peer_port, -1) <= 0) {
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
	if (res->peer_res0 == NULL) {
		res->peer_res0 = resolve_addr(peer, res->peer_port);
		if (res->peer_res0 == NULL) {
			acl_msg_error("%s(%d): resolve %s|%s error",
				__FUNCTION__, __LINE__, peer, res->peer_port);
			return -1;
		}
	}

	if (local != NULL) {
		/* First, check if the local is IP address. */
		res->local_res0 = try_numeric_addr(PF_UNSPEC, local, "0",
			&res->local_buf, &res->local_in);
		if (res->local_res0 == NULL) {
			/* Try to resolve the address from nameserver. */
			res->local_res0 = resolve_addr(local, "0");
		}
	}
	return 0;
}

ACL_SOCKET acl_inet_timed_connect(const char *addr, int blocking,
	int timeout, unsigned *flags)
{
	ACL_SOCKET  sock;
	struct addr_res ares;
	struct addrinfo *res;

	if (flags) {
		*flags = 0;
	}

	/* we should fill the ares with 0 to init all members int it. */
	memset(&ares, 0, sizeof(ares));

	if (parse_addr(addr, &ares) == -1) {
		return ACL_SOCKET_INVALID;
	}

	sock = ACL_SOCKET_INVALID;

	for (res = ares.peer_res0; res != NULL ; res = res->ai_next) {
		sock = connect_one(res, &ares, blocking, timeout, flags);
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
