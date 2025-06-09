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
#include "net/acl_valid_hostname.h"
#include "net/acl_vstream_net.h"

#endif

ACL_VSTREAM *acl_vstream_listen_ex(const char *addr, int qlen,
	unsigned flag, int io_bufsize, int io_timeout)
{
	ACL_SOCKET   listenfd;
	ACL_VSTREAM *sstream;
	char         buf[256];

	if (addr == 0 || *addr == 0 || qlen <= 0) {
		acl_msg_error("%s: input invalid", __FUNCTION__);
		return NULL;
	}

#ifdef ACL_UNIX
	/* This maybe unix addr, such as '/home/test/listen.sock' */
	if (acl_valid_unix(addr)) {
		listenfd = acl_unix_listen(addr, qlen, 0);
		if (listenfd == ACL_SOCKET_INVALID) {
			return NULL;
		}

		acl_non_blocking(listenfd, flag & ACL_INET_FLAG_NBLOCK ?
			ACL_NON_BLOCKING : ACL_BLOCKING);
		sstream = acl_vstream_fdopen(listenfd,
			ACL_VSTREAM_FLAG_RW, io_bufsize,
			io_timeout, ACL_VSTREAM_TYPE_LISTEN_UNIX);

		if (sstream == NULL) {
			acl_socket_close(listenfd);
			acl_msg_error("%s: open vstream error, addr(%s)",
				__FUNCTION__, addr);
			return NULL;
		}

		acl_vstream_set_local(sstream, addr);
		return sstream;
	}
#endif
	/* addr such as '192.168.0.1|80' */
	listenfd = acl_inet_listen(addr, qlen, flag);
	if (listenfd == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: listen addr(%s) error(%s)",
			__FUNCTION__, addr, acl_last_serror());
		return NULL;
	}
	sstream = acl_vstream_fdopen(listenfd, ACL_VSTREAM_FLAG_RW,
		io_bufsize, io_timeout, ACL_VSTREAM_TYPE_LISTEN_INET);

	if (acl_getsockname(listenfd, buf, sizeof(buf)) == -1) {
		acl_msg_warn("%s: getsockname error(%s) for sock(%d)",
			__FUNCTION__, acl_last_serror(), listenfd);
		acl_vstream_set_local(sstream, addr);
	} else  {
		acl_vstream_set_local(sstream, buf);
	}

	return sstream;
}

ACL_VSTREAM *acl_vstream_listen(const char *addr, int qlen)
{
	return acl_vstream_listen_ex(addr, qlen, ACL_INET_FLAG_NONE, 0, 0);
}

ACL_VSTREAM *acl_vstream_accept_ex(ACL_VSTREAM *sstream,
	ACL_VSTREAM *cstream, char *addr, int size)
{
	ACL_SOCKET connfd = ACL_SOCKET_INVALID;
	ACL_SOCKET servfd = ACL_VSTREAM_SOCK(sstream);
	char buf[256];

	if (sstream->read_ready)
		sstream->read_ready = 0;

#if defined(ACL_UNIX)
	connfd = acl_accept(servfd, buf, sizeof(buf), NULL);
#elif defined(ACL_WINDOWS)
	if (!(sstream->type & ACL_VSTREAM_TYPE_LISTEN_IOCP)) {
		connfd = acl_accept(servfd, buf, sizeof(buf), NULL);
	} else if (sstream->iocp_sock == ACL_SOCKET_INVALID) {
		return NULL;
	} else {
		int   ret;

		connfd = sstream->iocp_sock;
		sstream->iocp_sock = ACL_SOCKET_INVALID;

		/* iocp 方式下，需调用下面过程以允许调用
		 * getpeername/getsockname
		 */
		ret = setsockopt(connfd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
			(char *)&servfd, sizeof(servfd));
		if (ret == SOCKET_ERROR) {
			buf[0] = 0;
		} else {
			acl_getpeername(connfd, buf, sizeof(buf));
		}
	}
#else
#error "unknown os"
#endif

	if (connfd == ACL_SOCKET_INVALID) {
		return NULL;
	}

	if (addr != NULL && size > 0) {
		ACL_SAFE_STRNCPY(addr, buf, size);
	}

	if (cstream == NULL) {
		cstream = acl_vstream_fdopen(connfd,
			ACL_VSTREAM_FLAG_RW,
			(int) sstream->read_buf_len,
			sstream->rw_timeout,
			ACL_VSTREAM_TYPE_SOCK);

		/* 让 cstream 的 context 成员继承 sstream 的 context 成员. */
		cstream->context = sstream->context;
	} else {
		acl_vstream_reset(cstream);
		ACL_VSTREAM_SET_SOCK(cstream, connfd);
	}

	if (sstream->sa_local) {
		acl_vstream_set_local_addr(cstream, sstream->sa_local);
	}
	acl_vstream_set_peer(cstream, buf);
	return cstream;
}

ACL_VSTREAM *acl_vstream_accept(ACL_VSTREAM *sstream, char *ipbuf, int bsize)
{
	return acl_vstream_accept_ex(sstream, NULL, ipbuf, bsize);
}

ACL_VSTREAM *acl_vstream_timed_connect(const char *addr, int block_mode,
	int conn_timeout, int rw_timeout, int rw_bufsize, unsigned *flags)
{
	ACL_VSTREAM *client;
	ACL_SOCKET fd;
	int  family = 0;
	char buf[256];
	unsigned f = 0;

	if (addr == NULL || *addr == 0) {
		acl_msg_error("%s(%d): addr null", __FUNCTION__, __LINE__);
		return NULL;
	}

#if defined(ACL_UNIX)
	if (acl_valid_unix(addr)) {
		fd = acl_unix_connect(addr, block_mode, conn_timeout);
	} else
#endif
	{
		fd = acl_inet_timed_connect(addr, block_mode, conn_timeout, &f);
	}

	if (flags) {
		*flags = f;
	}

	if (fd == ACL_SOCKET_INVALID) {
		return NULL;
	}

	client = acl_vstream_fdopen(fd, ACL_VSTREAM_FLAG_RW,
		    rw_bufsize, rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	if (client == NULL) {
		acl_socket_close(fd);
		return NULL;
	}

	/* must set the IO timeout in ms unit */
	ACL_VSTREAM_SET_MS(client);

	family = acl_getsockfamily(fd);

	switch (family) {
#ifdef ACL_UNIX
	case AF_UNIX:
		client->type |= ACL_VSTREAM_TYPE_UNIX;
		break;
#endif
	case AF_INET:
		client->type |= ACL_VSTREAM_TYPE_INET4;
		break;
#ifdef AF_INET6
	case AF_INET6:
		client->type |= ACL_VSTREAM_TYPE_INET6;
		break;
#endif
	default:
		break;
	}

	if (acl_getpeername(ACL_VSTREAM_SOCK(client), buf, sizeof(buf)) < 0) {
		acl_vstream_set_peer(client, addr);
	} else {
		acl_vstream_set_peer(client, buf);
	}

	if (f & ACL_CONNECT_F_BIND_IP_OK) {
		client->flag |= ACL_VSTREAM_FLAG_BIND_IP_OK;
	}

	if (f & ACL_CONNECT_F_BIND_IFACE_OK) {
		client->flag |= ACL_VSTREAM_FLAG_BIND_IFACE_OK;
	}
	return client;
}

ACL_VSTREAM *acl_vstream_connect2(const char *addr,
	int block_mode, int conn_timeout, int rw_timeout,
	int rw_bufsize, unsigned *flags)
{
	ACL_VSTREAM *conn = acl_vstream_timed_connect(addr, block_mode,
		conn_timeout * 1000, rw_timeout * 1000,
		rw_bufsize, flags);

	/* reset rw_timeout with second unit */
	if (conn) {
		ACL_VSTREAM_CLR_MS(conn);
		ACL_VSTREAM_SET_RWTIMO(conn, rw_timeout);
	}
	return conn;
}

ACL_VSTREAM *acl_vstream_connect(const char *addr, int block_mode,
	int connect_timeout, int rw_timeout, int rw_bufsize)
{
	return acl_vstream_connect2(addr, block_mode, connect_timeout,
			rw_timeout, rw_bufsize, NULL);
}

/****************************************************************************/

static int udp_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream, void *arg acl_unused)
{
	ACL_SOCKADDR sa;
	socklen_t sa_len = sizeof(sa);
	int   ret;
	size_t n;

	if (stream->sa_peer_size == 0) {
		acl_vstream_set_peer(stream, "0.0.0.0|0");
	}

	memset(&sa, 0, sizeof(sa));

	if (stream->read_ready) {
		stream->read_ready = 0;
	} else if (stream->rw_timeout >= 0
		&& acl_read_wait(fd, stream->rw_timeout) < 0) {
		return -1;
	}

	ret = (int) recvfrom(fd, buf, (int) size, 0,
			(struct sockaddr*) &sa, &sa_len);

	if (ret <= 0) {
		return ret;
	}

	if (sa.sa.sa_family == 0) {
#ifdef ACL_UNIX
		n = stream->sa_local_len > stream->sa_peer_len ?
			stream->sa_peer_len : stream->sa_local_len;
		if (n > 0 && stream->sa_local
			  && stream->sa_local->sa_family == AF_UNIX) {
			acl_vstream_set_peer_addr(stream, stream->sa_local);
		}
#endif
		return ret;
	}
	
	if (sa_len > 0) {
		n = stream->sa_peer_size > (size_t) sa_len ?
			(size_t) sa_len : stream->sa_peer_size;
	} else {
		n = stream->sa_peer_size > sizeof(sa) ?
			sizeof(sa) : stream->sa_peer_size;
	}

	if (!stream->sa_peer || memcmp(stream->sa_peer, &sa, n) != 0) {
		acl_vstream_set_peer_addr(stream, (struct sockaddr *) &sa);
	}
	return ret;
}

static int udp_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream, void *arg acl_unused)
{
	int   ret;

	if (stream->sa_peer_len == 0) {
		acl_msg_error("%s, %s(%d): peer addr null",
			__FUNCTION__, __FILE__, __LINE__);
		return -1;
	}

	ret = (int) sendto(fd, buf, (int) size, 0,
			(struct sockaddr*) stream->sa_peer,
			(int) stream->sa_peer_len);
	return ret;
}

ACL_VSTREAM *acl_vstream_bind(const char *addr, int rw_timeout, unsigned flag)
{
	ACL_VSTREAM *stream;
	ACL_SOCKET   sock = acl_udp_bind(addr, flag);
	ACL_SOCKADDR saddr;
	socklen_t    len = sizeof(saddr);
	
	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: bind addr %s error %s", __FILE__,
			__LINE__, __FUNCTION__, addr, acl_last_serror());
		return NULL;
	}

	stream = acl_vstream_fdopen(sock, O_RDWR, 4096, -1, ACL_VSTREAM_TYPE_SOCK);
	stream->rw_timeout = rw_timeout;

	/* 设置本地绑定地址 */
	if (getsockname(sock, &saddr.sa, &len) == 0) {
		acl_vstream_set_local_addr(stream, &saddr.sa);
	}

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

/****************************************************************************/

static int join_multicast(ACL_SOCKET sock, const char *addr,
	const char *iface, int enable_loop)
{
	int on;
	struct ip_mreq mreq;

	memset(&mreq, 0, sizeof(mreq));

	mreq.imr_multiaddr.s_addr = inet_addr(addr);
	mreq.imr_interface.s_addr = strcmp(iface, "0.0.0.0") == 0 ?
		htonl(INADDR_ANY) : inet_addr(iface);

	if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		 (const char*) &mreq, sizeof(mreq)) == -1) {
		acl_msg_error("%s(%d), %s: setsockopt %d error %s", __FILE__,
			__LINE__, __FUNCTION__, (int) sock, acl_last_serror());
		return -1;
	}

	on = enable_loop;
	if (setsockopt(sock,IPPROTO_IP, IP_MULTICAST_LOOP,
		 (const char*) &on, sizeof(on)) == -1) {
		acl_msg_error("%s(%d), %s: setsockopt IP_MULTICAST_LOOP error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}

	return 0;
}

static int multicast_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream, void *arg acl_unused)
{
	return udp_read(fd, buf, size, timeout, stream, arg);
}

static int multicast_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *stream, void *arg acl_unused)
{
	int   ret;

	if (stream->sa_local_len == 0) {
		acl_msg_error("%s, %s(%d): peer addr null",
			__FUNCTION__, __FILE__, __LINE__);
		return -1;
	}

	/* 与 UDP 单发不同之处使用了 local 保存的地址做为目标地址，因为其存放着组播地址 */
	ret = (int) sendto(fd, buf, (int) size, 0,
			(struct sockaddr*) stream->sa_local,
			(int) stream->sa_local_len);
	return ret;
}

ACL_VSTREAM *acl_vstream_bind_multicast(const char *addr, const char *iface,
	int port, int timeout, unsigned flag)
{
	ACL_SOCKET   sock;
	ACL_VSTREAM *stream;
	char addrbuf[256];
	int enable_loop = (flag & ACL_INET_FLAG_MULTILOOP_ON) ? 1 : 0;

#if defined(_WIN32) || defined(_WIN64)
	acl_snprintf(addrbuf, sizeof(addrbuf), "%s|%d", iface, port);
#else
	acl_snprintf(addrbuf, sizeof(addrbuf), "%s|%d", addr, port);
#endif

	sock = acl_udp_bind(addrbuf, flag);
	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: bind addr %s error %s", __FILE__,
			__LINE__, __FUNCTION__, addrbuf, acl_last_serror());
		return NULL;
	}

	if (join_multicast(sock, addr, iface, enable_loop) == -1) {
		acl_socket_close(sock);
		return NULL;
	}

	stream = acl_vstream_fdopen(sock, O_RDWR, 4096, -1, ACL_VSTREAM_TYPE_SOCK);
	stream->rw_timeout = timeout;

	/* 设置本地绑定地址，该地址同时用做外发地址 */
	acl_snprintf(addrbuf, sizeof(addrbuf), "%s|%d", addr, port);
	acl_vstream_set_local(stream, addrbuf);

	acl_vstream_ctl(stream,
		ACL_VSTREAM_CTL_READ_FN, multicast_read,
		ACL_VSTREAM_CTL_WRITE_FN, multicast_write,
		ACL_VSTREAM_CTL_CONTEXT, stream->context,
		ACL_VSTREAM_CTL_END);
	return stream;
}

int acl_multicast_set_ttl(ACL_SOCKET sock, int ttl)
{
	if (ttl < 0) {
		acl_msg_error("%s: invalid ttl=%d", __FUNCTION__, ttl);
		return -1;
	}

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: invalid socket", __FUNCTION__ );
		return -1;
	}

	if (setsockopt(sock,  IPPROTO_IP,IP_TTL,
	       (const char*) &ttl, sizeof(ttl)) == -1) {
		acl_msg_error("%s: setsockopt IP_TTL error %s, sock=%d",
		     __FUNCTION__, acl_last_serror(), sock);
		return -1;
	}
	return 0;
}

int acl_multicast_set_if(ACL_SOCKET sock, const char *addr)
{
	struct in_addr in;
	if (addr == NULL || *addr == 0) {
		acl_msg_error("%s: addr null", __FUNCTION__);
		return -1;
	}

	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: invalid socket", __FUNCTION__);
		return -1;
	}

	in.s_addr = inet_addr(addr);

	if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF,
	       (const char*) &in, sizeof(in)) < 0) {
		acl_msg_error("%s: set IP_MULTICAST_IF error=%s, addr=%s",
		     __FUNCTION__, acl_last_serror(), addr);
		return -1;
	}

	return 0;
}

int acl_multicast_drop(ACL_SOCKET sock, const char *addr, const char *iface)
{
	if (sock == ACL_SOCKET_INVALID) {
		acl_msg_error("%s: invalid socket", __FUNCTION__);
		return -1;
	}

	struct ip_mreq ipmr;
	memset(&ipmr, 0, sizeof(ipmr));

	ipmr.imr_interface.s_addr = strcmp(iface, "0.0.0.0") == 0 ?
		htonl(INADDR_ANY) : inet_addr(iface);
	ipmr.imr_multiaddr.s_addr = inet_addr(addr);

	if (setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
	       (const char*)&ipmr, sizeof(ipmr)) == -1) {
		acl_msg_error("%s: set IP_DROP_MEMBERSHIP error=%s",
			__FUNCTION__, acl_last_serror());
		return -1;
	}
	return 0;
}
