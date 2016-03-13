#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef  HP_UX
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED  1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef  ACL_WINDOWS
#include <io.h>
#include <fcntl.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "net/acl_sane_inet.h"
#include "net/acl_sane_socket.h"
#include "net/acl_connect.h"
#include "net/acl_listen.h"
#include "net/acl_host_port.h"
#include "net/acl_vstream_net.h"

#endif

ACL_VSTREAM *acl_vstream_listen_ex(const char *addr, int qlen,
	int block_mode, int io_bufsize, int io_timeout)
{
	const char *myname = "acl_vstream_listen_ex";
	ACL_SOCKET  listenfd;
	struct sockaddr_in local;
	ACL_VSTREAM *listen_stream;
	int   len;

	if (addr == 0 || *addr == 0 || qlen <= 0) {
		acl_msg_error("%s: input invalid", myname);
		return NULL;
	}

#ifdef	ACL_UNIX
	/* this maybe unix addr, such as '/home/test/listen.sock' */
	if (strchr(addr, '/') != NULL) {
		listenfd = acl_unix_listen(addr, qlen, 0);
		if (listenfd == ACL_SOCKET_INVALID)
			return NULL;
		acl_non_blocking(listenfd, block_mode);
		listen_stream = acl_vstream_fdopen(listenfd,
			ACL_VSTREAM_FLAG_RW, io_bufsize,
			io_timeout, ACL_VSTREAM_TYPE_LISTEN_UNIX);

		if (listen_stream == NULL) {
			acl_socket_close(listenfd);
			acl_msg_error("%s: open vstream error, addr(%s)",
				myname, addr);
			return NULL;
		}

		acl_vstream_set_local(listen_stream, addr);

		sprintf(listen_stream->errbuf, "+OK");
		return (listen_stream);
	}
#endif
	/* addr such as '192.168.0.1:80' */
	listenfd = acl_inet_listen(addr, qlen, block_mode);
	if (listenfd == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: listen addr(%s) error(%s)",
			myname, addr, acl_last_serror());
		return NULL;
	}
	listen_stream = acl_vstream_fdopen(listenfd,
		ACL_VSTREAM_FLAG_RW, io_bufsize,
		io_timeout, ACL_VSTREAM_TYPE_LISTEN_INET);
	if (listen_stream == NULL) {
		acl_socket_close(listenfd);
		acl_msg_error("%s: open vstream error addr(%s)", myname, addr);
		return NULL;
	}

	memset(&local, 0, sizeof(local));
	len = (int) sizeof(struct sockaddr);
	if (getsockname(listenfd, (struct sockaddr*) &local,
		(socklen_t *) &len) < 0)
	{
		acl_msg_warn("%s: getsockname error(%s) for sock(%d)",
			myname, acl_last_serror(), listenfd);
		acl_vstream_set_local(listen_stream, addr);
	} else {
		char  ip[32], buf[64];
		int   port;

		acl_inet_ntoa(local.sin_addr, ip, sizeof(ip));
		port = ntohs(local.sin_port);
		snprintf(buf, sizeof(buf), "%s:%d", ip, port);
		acl_vstream_set_local(listen_stream, buf);
	}

	sprintf(listen_stream->errbuf, "+OK");
	return listen_stream;
}

ACL_VSTREAM *acl_vstream_listen(const char *addr, int qlen)
{
	return acl_vstream_listen_ex(addr, qlen, ACL_BLOCKING, 0, 0);
}

ACL_VSTREAM *acl_vstream_accept_ex(ACL_VSTREAM *listen_stream,
	ACL_VSTREAM *client_stream, char *addr, int size)
{
	const char *myname = "acl_vstream_accept_ex";
	ACL_SOCKET connfd = ACL_SOCKET_INVALID;
	ACL_SOCKET servfd = ACL_VSTREAM_SOCK(listen_stream);
	char buf[256];

	if (listen_stream->read_ready)
		listen_stream->read_ready = 0;

	if ((listen_stream->type & ACL_VSTREAM_TYPE_LISTEN_INET)) {
#ifdef ACL_WINDOWS
		if (!(listen_stream->type & ACL_VSTREAM_TYPE_LISTEN_IOCP))
			connfd = acl_inet_accept_ex(servfd, buf, sizeof(buf));
		else if (listen_stream->iocp_sock == ACL_SOCKET_INVALID)
			return NULL;
		else {
			int   ret;

			connfd = listen_stream->iocp_sock;
			listen_stream->iocp_sock = ACL_SOCKET_INVALID;

			/* iocp 方式下，需调用下面过程以允许调用
			 * getpeername/getsockname
			 */
			ret = setsockopt(connfd, SOL_SOCKET,
				SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&servfd, sizeof(servfd));
			buf[0] = 0;
			if (ret != SOCKET_ERROR)
				acl_getpeername(connfd, buf, sizeof(buf));
		}
#else
		connfd = acl_inet_accept_ex(servfd, buf, sizeof(buf));
#endif

		if (connfd != ACL_SOCKET_INVALID && addr != NULL && size > 0)
			ACL_SAFE_STRNCPY(addr, buf, size);
#ifdef	ACL_UNIX
	} else if ((listen_stream->type & ACL_VSTREAM_TYPE_LISTEN_UNIX)) {
		connfd = acl_unix_accept(servfd);
		if (acl_getpeername(connfd, buf, sizeof(buf)) < 0)
			buf[0] = 0;
		if (addr)
			ACL_SAFE_STRNCPY(addr, buf, size);
#endif
	} else
		acl_msg_fatal("%s(%d)->%s: invalid listen stream(%d)",
			__FILE__, __LINE__, myname, listen_stream->flag);

	if (connfd == ACL_SOCKET_INVALID)
		return NULL;

	if (client_stream != NULL) {
		acl_vstream_reset(client_stream);
		ACL_VSTREAM_SET_SOCK(client_stream, connfd);
	} else {
		client_stream = acl_vstream_fdopen(connfd,
			ACL_VSTREAM_FLAG_RW,
			(int) listen_stream->read_buf_len,
			listen_stream->rw_timeout,
			ACL_VSTREAM_TYPE_SOCK);
		/* 让 client_stream 的 context 成员继承 listen_stream 的
		 * context 成员变量.
		 */
		client_stream->context = listen_stream->context;
	}
	if (client_stream == NULL)
		return NULL;

	acl_vstream_set_peer(client_stream, buf);
	return client_stream;
}

ACL_VSTREAM *acl_vstream_accept(ACL_VSTREAM *listen_stream,
	char *ipbuf, int bsize)
{
	return acl_vstream_accept_ex(listen_stream, NULL, ipbuf, bsize);
}

ACL_VSTREAM *acl_vstream_connect_ex(const char *addr,
	int block_mode, int connect_timeout, int rw_timeout,
	int rw_bufsize, int *he_errorp)
{
	const char *myname = "acl_vstream_connect_ex";
	ACL_VSTREAM *client;
	ACL_SOCKET connfd;
	char *ptr, buf[256];

	if (addr == NULL || *addr == 0)
		acl_msg_fatal("%s: addr null", myname);

	ptr = strchr(addr, ':');
	if (ptr != NULL) {
		connfd = acl_inet_connect_ex(addr, block_mode,
			connect_timeout, he_errorp);
	}
#ifdef ACL_WINDOWS
	else {
		acl_msg_error("%s(%d): addr(%s) invalid",
			myname, __LINE__, addr);
		return NULL;
	}
#elif defined(ACL_UNIX)
	else {
		connfd = acl_unix_connect(addr, block_mode, connect_timeout);
	}
#else
	else {
		connfd = ACL_SOCKET_INVALID;
	}
#endif

	if (connfd == ACL_SOCKET_INVALID)
		return NULL;
	client = acl_vstream_fdopen(connfd, ACL_VSTREAM_FLAG_RW,
			rw_bufsize, rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	if (client == NULL) {
		acl_socket_close(connfd);
		return NULL;
	}

	if (acl_getpeername(ACL_VSTREAM_SOCK(client), buf, sizeof(buf)) < 0)
		acl_vstream_set_peer(client, addr);
	else
		acl_vstream_set_peer(client, buf);

	return client;
}

ACL_VSTREAM *acl_vstream_connect(const char *addr, int block_mode,
	int connect_timeout, int rw_timeout, int rw_bufsize)
{
	return acl_vstream_connect_ex(addr, block_mode, connect_timeout,
			rw_timeout, rw_bufsize, NULL);
}

static int udp_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream, void *arg acl_unused)
{
	struct sockaddr_in sa;
	socklen_t sa_len = sizeof(struct sockaddr_in);
	int   ret;

	if (stream->sa_peer_size == 0)
		acl_vstream_set_peer(stream, "0.0.0.0:0");

	memset(&sa, 0, sizeof(sa));

	if (stream->read_ready)
		stream->read_ready = 0;

	ret = recvfrom(fd, buf, (int) size, 0, (struct sockaddr*) &sa, &sa_len);

	if (ret > 0 && memcmp(stream->sa_peer, &sa, sizeof(sa)) != 0)
		acl_vstream_set_peer_addr(stream, &sa);
	return ret;
}

static int udp_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream, void *arg acl_unused)
{
	const char *myname = "udp_write";
	int   ret;

	if (stream->sa_peer_len == 0)
		acl_msg_fatal("%s, %s(%d): peer addr null",
			myname, __FILE__, __LINE__);

	ret = sendto(fd, buf, (int) size, 0,
		(struct sockaddr*) stream->sa_peer,
		(int) stream->sa_peer_len);
	return ret;
}

ACL_VSTREAM *acl_vstream_bind(const char *addr, int rw_timeout)
{
	const char *myname = "acl_vstream_bind";
	char *buf = acl_mystrdup(addr), addr_buf[256];
	char *host = NULL, *sport = NULL;
	const char *ptr;
	int   port;
	struct sockaddr_in sa;
	ACL_VSTREAM *stream;
	ACL_SOCKET sock;

	ptr = acl_host_port(buf, &host, "", &sport, (char *) 0);
	if (ptr != NULL) {
		acl_msg_error("%s(%d): %s, %s invalid",
			myname, __LINE__, addr, ptr);
		acl_myfree(buf);
		return NULL;
	}

	if (host && *host == 0)
		host = 0;
	if (host == NULL)
		host = "0.0.0.0";

	if (sport == NULL) {
		acl_msg_error("%s(%d): no port given from addr(%s)",
			myname, __LINE__, addr);
		acl_myfree(buf);
		return NULL;
	} else
		port = atoi(sport);

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = inet_addr(host);

#ifdef ACL_WINDOWS
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
#else
	sock = socket(AF_INET, SOCK_DGRAM, 0);
#endif

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: socket %s", myname, acl_last_serror());
		acl_myfree(buf);
		return NULL;
	}
	if (bind(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr)) < 0) {
		acl_msg_error("%s: bind %s  %s, %s",
			myname, addr, acl_last_serror(), strerror(errno));
		acl_socket_close(sock);
		acl_myfree(buf);
		return NULL;
	}

	stream = acl_vstream_fdopen(sock, O_RDWR, 4096, 0, ACL_VSTREAM_TYPE_SOCK);
	stream->rw_timeout = rw_timeout;

	if (port == 0)
		acl_getsockname(sock, addr_buf, sizeof(addr_buf));
	else
		snprintf(addr_buf, sizeof(addr_buf), "%s:%d", host, port);
	acl_myfree(buf);

	/* 设置本地绑定地址 */
	acl_vstream_set_local(stream, addr_buf);

	/* 注册流读写回调函数 */
	acl_vstream_ctl(stream,
		ACL_VSTREAM_CTL_READ_FN, udp_read,
		ACL_VSTREAM_CTL_WRITE_FN, udp_write,
		ACL_VSTREAM_CTL_CONTEXT, stream->context,
		ACL_VSTREAM_CTL_END);

	return stream;
}

void acl_vstream_set_udp_io(ACL_VSTREAM *stream)
{
	acl_vstream_ctl(stream,
		ACL_VSTREAM_CTL_READ_FN, udp_read,
		ACL_VSTREAM_CTL_WRITE_FN, udp_write,
		ACL_VSTREAM_CTL_CONTEXT, stream->context,
		ACL_VSTREAM_CTL_END);
}
