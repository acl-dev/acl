#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_sane_inet.h"
#include "net/acl_host_port.h"
#include "net/acl_sane_socket.h"
#include "net/acl_listen.h"

#endif

static ACL_SOCKET inet_listen(const char *addr, const struct addrinfo *res,
	int backlog, int blocking)
{
	const char *myname = "inet_listen";
	ACL_SOCKET sock;
	int on;

	sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: socket %s", myname, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_warn("%s: setsockopt(SO_REUSEADDR): %s",
			myname, acl_last_serror());
	}

#if defined(SO_REUSEPORT) && defined(USE_REUSEPORT)
	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_warn("%s: setsocket(SO_REUSEPORT): %s",
			myname, acl_last_serror());
	}
#endif

#if defined(TCP_FASTOPEN) && defined(USE_FASTOPEN)
	on = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_FASTOPEN,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_warn("%s: setsocket(TCP_FASTOPEN): %s",
			myname, acl_last_serror());
	}
#endif

#ifdef ACL_WINDOWS
	if (bind(sock, res->ai_addr, (int) res->ai_addrlen) < 0) {
#else
	if (bind(sock, res->ai_addr, res->ai_addrlen) < 0) {
#endif
		acl_msg_error("%s: bind %s error %s, addr=%s",
			myname, addr, acl_last_serror(), addr);
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	acl_non_blocking(sock, blocking);

	if (listen(sock, backlog) < 0) {
		acl_msg_error("%s: listen error: %s, addr=%s",
			myname, acl_last_serror(), addr);
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	acl_msg_info("%s: listen %s ok", myname, addr);

	return sock;
}

/* acl_inet_listen - create TCP listener */

ACL_SOCKET acl_inet_listen(const char *addr, int backlog, int blocking)
{
	const char *myname = "acl_inet_listen";
	char *buf, *host = NULL, *port = NULL;
	const char *ptr;
	struct addrinfo hints, *res0, *res;
	ACL_SOCKET sock;
	int err;

	/*
	 * Translate address information to internal form.
	 */
	buf = acl_mystrdup(addr);
	ptr = acl_host_port(buf, &host, "", &port, (char *) 0);
	if (ptr) {
		acl_msg_error("%s(%d): %s, %s invalid",
			myname, __LINE__, addr, ptr);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	}

	if (host && *host == 0)
		host = 0;
	if (host == NULL)
		host = "0";

	if (port == NULL) {
		acl_msg_error("%s(%d): no port given from addr(%s)",
			myname, __LINE__, addr);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	} else if (atoi(port) < 0) {
		acl_msg_error("%s: port(%s) < 0 invalid from addr(%s)",
			myname, port, addr);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
#ifdef	ACL_MACOSX
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

	if ((err = getaddrinfo(host, port, &hints, &res0))) {
		acl_msg_error("%s(%d), %s: getaddrinfo error %s, host=%s",
			__FILE__, __LINE__, myname, gai_strerror(err), host);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	}

	acl_myfree(buf);

	sock = ACL_SOCKET_INVALID;

	for (res = res0; res != NULL; res = res->ai_next) {
		sock = inet_listen(addr, res, backlog, blocking);
		if (sock != ACL_SOCKET_INVALID)
			break;
	}

	if (res0)
		freeaddrinfo(res0);

	return sock;
}

ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd)
{
	return acl_inet_accept_ex(listen_fd, NULL, 0);
}

ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf, size_t size)
{
	struct sockaddr_storage sa;
	socklen_t len = sizeof(sa);
	ACL_SOCKET fd;

	memset(&sa, 0, sizeof(sa));

	/* when client_addr not null and protocol is AF_INET, acl_sane_accept
	 * will set nodelay on the accepted socket, 2008.9.4, zsx
	 */
	fd = acl_sane_accept(listen_fd, (struct sockaddr *)&sa, &len);
	if (fd == ACL_SOCKET_INVALID)
		return fd;

	if (ipbuf != NULL && size > 0 && acl_getpeername(fd, ipbuf, size) < 0)
		ipbuf[0] = 0;

	return fd;
}
