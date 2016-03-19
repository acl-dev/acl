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

/* acl_inet_listen - create TCP listener */

ACL_SOCKET acl_inet_listen(const char *addr, int backlog, int block_mode)
{
	const char *myname = "acl_inet_listen";
	ACL_SOCKET sock;
	int   on, nport;
	char *buf, *host = NULL, *sport = NULL;
	const char *ptr;
	struct sockaddr_in sa;

	/*
	 * Translate address information to internal form.
	 */
	buf = acl_mystrdup(addr);
	ptr = acl_host_port(buf, &host, "", &sport, (char *) 0);
	if (ptr) {
		acl_msg_error("%s(%d): %s, %s invalid", myname, __LINE__, addr, ptr);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	}

	if (host && *host == 0)
		host = 0;
	if (host == NULL)
		host = "0.0.0.0";

	if (sport == NULL) {
		acl_msg_error("%s(%d): no port given from addr(%s)", myname, __LINE__, addr);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	}
	nport = atoi(sport);
	if (nport < 0) {
		acl_msg_error("%s: port(%d) < 0 invalid from addr(%s)",
			myname, nport, addr);
		acl_myfree(buf);
		return ACL_SOCKET_INVALID;
	}

	memset(&sa, 0, sizeof(sa));
	sa.sin_family      = AF_INET;
	sa.sin_port        = htons((short) nport);
	sa.sin_addr.s_addr = inet_addr(host);

	acl_myfree(buf);

	/* Create a listener socket. */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: socket %s", myname, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_error("%s: setsockopt(SO_REUSEADDR): %s",
			myname, acl_last_serror());
	}

#if defined(SO_REUSEPORT) && defined(USE_REUSEPORT)
	on = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_error("%s: setsocket(SO_REUSEPORT): %s",
			myname, acl_last_serror());
	}
#endif

#if defined(TCP_FASTOPEN) && defined(USE_FASTOPEN)
	on = 1;
	if (setsockopt(sock, IPPROTO_TCP, TCP_FASTOPEN,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_error("%s: setsocket(TCP_FASTOPEN): %s",
			myname, acl_last_serror());
	}
#endif

	if (bind(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
		acl_msg_error("%s: bind %s error %s",
			myname, addr, acl_last_serror());
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	acl_non_blocking(sock, block_mode);

	if (listen(sock, backlog) < 0) {
		acl_msg_error("%s: listen error: %s, addr(%s)",
			myname, acl_last_serror(), addr);
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}

	return sock;
}

ACL_SOCKET acl_inet_accept(ACL_SOCKET listen_fd)
{
	return acl_inet_accept_ex(listen_fd, NULL, 0);
}

ACL_SOCKET acl_inet_accept_ex(ACL_SOCKET listen_fd, char *ipbuf, size_t size)
{
	struct sockaddr_in client_addr;
	socklen_t addr_len;
	ACL_SOCKET fd;

	memset(&client_addr, 0, sizeof(client_addr));
	addr_len = sizeof(client_addr);

	/* when client_addr not null and protocol is AF_INET, acl_sane_accept
	 * will set nodelay on the accepted socket, 2008.9.4, zsx
	 */
	fd = acl_sane_accept(listen_fd, (struct sockaddr *)&client_addr, &addr_len);
	if (fd == ACL_SOCKET_INVALID)
		return fd;
	if (ipbuf != NULL && size > 0 && acl_getpeername(fd, ipbuf, size) < 0)
		ipbuf[0] = 0;

	return fd;
}
