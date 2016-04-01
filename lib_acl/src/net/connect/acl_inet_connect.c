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
#include "net/acl_connect.h"

#endif

static ACL_SOCKET inet_connect_one(const char *ip, int port,
	const char *local_ip, int b_mode, int timeout);

/* acl_inet_connect - connect to TCP listener */

ACL_SOCKET acl_inet_connect(const char *addr, int block_mode, int timeout)
{
	int   h_error = 0;

	return (acl_inet_connect_ex(addr, block_mode, timeout, &h_error));
}

ACL_SOCKET acl_inet_connect_ex(const char *addr, int b_mode,
	int timeout, int *h_error)
{
	const char *myname = "acl_inet_connect_ex";
	ACL_SOCKET  sock = ACL_SOCKET_INVALID;
	char  buf[256], *ptr;
	const char *ip, *remote, *local_ip;
	int   port, i, n;
	ACL_DNS_DB *h_dns_db;

	if (h_error)
		*h_error = 0;

	snprintf(buf, sizeof(buf) - 1, "%s", addr);
	ptr = strchr(buf, ':');
	if (ptr == NULL) {
		acl_msg_error("%s, %s(%d): invalid addr(%s)",
				__FILE__, myname, __LINE__, addr);
		return (ACL_SOCKET_INVALID);
	}

	*ptr++ = 0;
	port = atoi(ptr);
	if (port <= 0) {
		acl_msg_error("%s, %s(%d): invalid port(%d)",
				__FILE__, myname, __LINE__, port);
		return (ACL_SOCKET_INVALID);
	}

	ptr = strchr(buf, '@');
	if (ptr != NULL) {
		*ptr++ = 0;
		local_ip = buf;
		remote = ptr; 
	} else {
		local_ip = NULL;
		remote = buf;
	}

	if (strlen(remote) == 0) {
		acl_msg_error("%s, %s(%d): ip buf's length is 0",
					__FILE__, myname, __LINE__);
		return (ACL_SOCKET_INVALID);
	}

	h_dns_db = acl_gethostbyname(remote, h_error);
	if (h_dns_db == NULL) {
		n = h_error ? *h_error : -1;

		acl_msg_error("%s, %s(%d): gethostbyname error(%s), addr=%s",
			__FILE__, myname, __LINE__,
			acl_netdb_strerror(n), remote);
		return (ACL_SOCKET_INVALID);
	}

	sock = ACL_SOCKET_INVALID;
	n = acl_netdb_size(h_dns_db);

	for (i = 0; i < n; i++) {
		ip = acl_netdb_index_ip(h_dns_db, i);
		if (ip == NULL)
			break;

		sock = inet_connect_one(ip, port, local_ip, b_mode, timeout);
		if (sock != ACL_SOCKET_INVALID)
			break;
		acl_msg_error("%s(%d): connect error: %s, addr: %s:%d, fd: %d",
			myname, __LINE__, acl_last_serror(), ip, port, sock);
	}

	acl_netdb_free(h_dns_db);

	return (sock);
}

/* inet_connect_one - try to connect to one address */

static ACL_SOCKET inet_connect_one(const char *ip, int port,
	const char *local_ip, int b_mode, int timeout)
{
	const char *myname = "inet_connect_one";
	ACL_SOCKET  sock;
	struct sockaddr_in saddr;

	/*
	 * Create a client socket.
	 */
	/* sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d): create socket error: %s",
			myname, __LINE__, acl_last_serror());
		return ACL_SOCKET_INVALID;
	}

	acl_tcp_set_rcvbuf(sock, ACL_SOCKET_RBUF_SIZE);
	acl_tcp_set_sndbuf(sock, ACL_SOCKET_WBUF_SIZE);

	if (local_ip != NULL && *local_ip != 0) {
		memset(&saddr, 0, sizeof(saddr));
		saddr.sin_family = AF_INET;
		saddr.sin_addr.s_addr = inet_addr(local_ip);
		if (bind(sock, (struct sockaddr *) &saddr,
			sizeof(struct sockaddr)) < 0)
		{
			acl_socket_close(sock);
			acl_msg_error("%s(%d): bind ip(%s) error: %s, fd: %d",
				myname, __LINE__, local_ip,
				acl_last_serror(), sock);
			return ACL_SOCKET_INVALID;
		}
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons((short) port);
	saddr.sin_addr.s_addr = inet_addr(ip);

	/*
	 * Timed connect.
	 */
	if (timeout > 0) {
		acl_non_blocking(sock, ACL_NON_BLOCKING);
		if (acl_timed_connect(sock, (const struct sockaddr *) &saddr,
			sizeof(struct sockaddr), timeout) < 0)
		{
			acl_socket_close(sock);
			return (ACL_SOCKET_INVALID);
		}
		if (b_mode != ACL_NON_BLOCKING)
			acl_non_blocking(sock, b_mode);
		return sock;
	}

	/*
	 * Maybe block until connected.
	 */
	acl_non_blocking(sock, b_mode);
	if (acl_sane_connect(sock, (const struct sockaddr *) &saddr,
		sizeof(saddr)) < 0)
	{
		int  ret, err, errnum;
		socklen_t len;

		errnum = acl_last_error();
		len = sizeof(err);
		ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, (char *) &err, &len);
		if (ret < 0) {
#ifdef  SUNOS5
			/*
			 * Solaris 2.4's socket emulation doesn't allow you
			 * to determine the error from a failed non-blocking
			 * connect and just returns EPIPE.  Create a fake
			 * error message for connect. -- fenner@parc.xerox.com
			 */
			if (errno == EPIPE)
				acl_set_error(ACL_ENOTCONN);
#endif
			acl_socket_close(sock);
			return ACL_SOCKET_INVALID;
		} else if (err != 0) {
			errnum = err;
			acl_set_error(err);
		}

#ifdef	ACL_WINDOWS
		if (errnum == ACL_EINPROGRESS || errnum == ACL_EWOULDBLOCK)
			return sock;
#elif defined(ACL_UNIX)
		if (errnum == ACL_EINPROGRESS || errnum == EISCONN)
			return sock;
#endif
		acl_socket_close(sock);
		return ACL_SOCKET_INVALID;
	}
	return sock;
}
