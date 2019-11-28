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
#include "stdlib/acl_iostuff.h"
#include "net/acl_host_port.h"
#include "net/acl_sane_inet.h"
#include "net/acl_sane_socket.h"
#include "net/acl_listen.h"

#endif

static ACL_SOCKET inet_listen(const char *addr, const struct addrinfo *res,
	int backlog, unsigned flag)
{
	ACL_SOCKET sock = acl_inet_bind(res, flag);

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: bind %s error %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

#if defined(TCP_FASTOPEN)
	if (flag & ACL_INET_FLAG_FASTOPEN) {
		int on = 1;
		int ret = setsockopt(sock, IPPROTO_TCP, TCP_FASTOPEN,
			(const void *) &on, sizeof(on));
		if (ret < 0)
			acl_msg_warn("%s(%d): setsocket(TCP_FASTOPEN): %s",
				__FUNCTION__, __LINE__, acl_last_serror());
	}
#endif

	acl_non_blocking(sock, flag & ACL_INET_FLAG_NBLOCK ?
		ACL_NON_BLOCKING : ACL_BLOCKING);

	if (listen(sock, backlog) < 0) {
		acl_socket_close(sock);
		acl_msg_error("%s(%d), %s: listen %s error %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	acl_msg_info("%s: listen %s ok", __FUNCTION__, addr);
	return sock;
}

/* acl_inet_listen - create TCP listener */

ACL_SOCKET acl_inet_listen(const char *addr, int backlog, unsigned flag)
{
	struct addrinfo *res0 = acl_host_addrinfo(addr, SOCK_STREAM), *res;
	ACL_SOCKET sock;

	if (res0 == NULL)
		return ACL_SOCKET_INVALID;

	sock = ACL_SOCKET_INVALID;

	for (res = res0; res != NULL; res = res->ai_next) {
		sock = inet_listen(addr, res, backlog, flag);
		if (sock != ACL_SOCKET_INVALID)
			break;
	}

	freeaddrinfo(res0);
	return sock;
}

ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd)
{
	return acl_inet_accept_ex(listen_fd, NULL, 0);
}

ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf, size_t size)
{
	ACL_SOCKADDR sa;
	socklen_t len = sizeof(sa);
	ACL_SOCKET fd;

	memset(&sa, 0, sizeof(sa));

	/* when client_addr not null and protocol is AF_INET, acl_sane_accept
	 * will set nodelay on the accepted socket, 2008.9.4, zsx
	 */
	fd = acl_sane_accept(listen_fd, (struct sockaddr*) &sa, &len);
	if (fd == ACL_SOCKET_INVALID)
		return fd;

	if (ipbuf != NULL && size > 0 && !acl_inet_ntop(&sa.sa, ipbuf, size))
		ipbuf[0] = 0;

	return fd;
}
