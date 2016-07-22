#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_UNIX
#include <sys/types.h>
#include <sys/socket.h>
# ifdef ACL_FREEBSD
#  include <netinet/in_systm.h>
#  include <netinet/in.h>
# endif
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdarg.h>
#include <string.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "net/acl_sane_socket.h"
#include "stdlib/acl_msg.h"
#include "net/acl_tcp_ctl.h"

#endif

void acl_tcp_set_rcvbuf(ACL_SOCKET fd, int size)
{
	const char *myname = "acl_tcp_set_rcvbuf";

	if (acl_getsocktype(fd) != AF_INET)
		return;

	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
		(char *) &size, sizeof(size)) < 0)
	{
		acl_msg_error("%s(%d): size(%d), setsockopt error(%s)",
			myname, __LINE__, size, acl_last_serror());
	}
}

void acl_tcp_set_sndbuf(ACL_SOCKET fd, int size)
{
	const char *myname = "acl_tcp_sndbuf";

	if (acl_getsocktype(fd) != AF_INET)
		return;

	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
		(char *) &size, sizeof(size)) < 0)
	{
		acl_msg_error("%s: FD %d, SIZE %d: %s\n",
			myname, fd, size, acl_last_serror());
	}
}

int acl_tcp_get_rcvbuf(ACL_SOCKET fd)
{
	const char *myname = "acl_tcp_get_rcvbuf";
	int   size;
	socklen_t len;

	if (acl_getsocktype(fd) != AF_INET)
		return 0;

	len = (socklen_t) sizeof(size);
	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF,
		(char *) &size, &len) < 0)
	{
		acl_msg_error("%s(%d): size(%d), getsockopt error(%s)",
			myname, __LINE__, size, acl_last_serror());
		return (-1);
	}

	return (size);
}

int acl_tcp_get_sndbuf(ACL_SOCKET fd)
{
	const char *myname = "acl_tcp_get_sndbuf";
	int   size;
	socklen_t len;

	if (acl_getsocktype(fd) != AF_INET)
		return 0;

	len = (socklen_t) sizeof(size);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, &len) < 0) {
		acl_msg_error("%s(%d): size(%d), getsockopt error(%s)",
			myname, __LINE__, size, acl_last_serror());
		return (-1);
	}

	return (size);
}

void acl_tcp_set_nodelay(ACL_SOCKET fd)
{
	acl_tcp_nodelay(fd, 1);
}

void acl_tcp_nodelay(ACL_SOCKET fd, int onoff)
{
	const char *myname = "acl_tcp_nodelay";
	int   on = onoff ? 1 : 0;

	if (acl_getsocktype(fd) != AF_INET)
		return;

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
		(char *) &on, sizeof(on)) < 0)
	{
		acl_msg_error("%s(%d): set nodelay error(%s), onoff(%d)",
			myname, __LINE__, acl_last_serror(), onoff);
	}
}

int acl_get_tcp_nodelay(ACL_SOCKET fd)
{
	const char *myname = "acl_get_tcp_nodelay";
	int  on = 0;
	socklen_t len;

	if (acl_getsocktype(fd) != AF_INET)
		return 0;

	len = (socklen_t) sizeof(on);

	if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, &len) < 0) {
		acl_msg_error("%s(%d): getsockopt error: %s, fd: %d",
			myname, __LINE__, acl_last_serror(), fd);
		return -1;
	}

	return on;
}

void acl_tcp_so_linger(ACL_SOCKET fd, int onoff, int timeout)
{
	const char *myname = "acl_tcp_so_linger";
	struct linger  l;

	if (acl_getsocktype(fd) != AF_INET)
		return;

	l.l_onoff = onoff ? 1 : 0;
	l.l_linger = timeout >= 0 ? timeout : 0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER,
		(char *) &l, sizeof(l)) < 0)
	{
		acl_msg_error("%s(%d): setsockopt(SO_LINGER) error(%s),"
			" onoff(%d), timeout(%d)", myname, __LINE__,
			acl_last_serror(), onoff, timeout);
	}
}

int acl_get_tcp_solinger(ACL_SOCKET fd)
{
	const char *myname = "acl_get_tcp_solinger";
	struct linger  l;
	socklen_t len = (socklen_t) sizeof(l);

	if (acl_getsocktype(fd) != AF_INET)
		return -1;

	memset(&l, 0, sizeof(l));
	if (getsockopt(fd, SOL_SOCKET, SO_LINGER, (char*) &l, &len) < 0) {
		acl_msg_error("%s(%d): getsockopt error: %s, fd: %d",
			myname, __LINE__, acl_last_serror(), fd);
		return -1;
	}

	return l.l_linger == 0 ? -1 : l.l_linger;
}

void acl_tcp_defer_accept(ACL_SOCKET fd, int timeout)
{
#ifdef	TCP_DEFER_ACCEPT
	const char *myname = "acl_tcp_defer_accept";

	if (timeout < 0)
		timeout = 0;
	if (setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT,
		&timeout, sizeof(timeout)) < 0)
	{
		acl_msg_error("%s: setsockopt(TCP_DEFER_ACCEPT): %s",
			myname, acl_last_serror());
	}
#else
	(void) fd;
	(void) timeout;
#endif
}

void acl_tcp_fastopen(ACL_SOCKET fd, int on)
{
#if defined(TCP_FASTOPEN)
	const char *myname = "acl_tcp_fastopen";

	if (on)
		on = 1;
	if (setsockopt(fd, IPPROTO_TCP, TCP_FASTOPEN,
		(const void *) &on, sizeof(on)) < 0)
	{
		acl_msg_error("%s: setsocket(TCP_FASTOPEN): %s",
			myname, acl_last_serror());
	}
#else
	(void) fd;
	(void) on;
#endif
}
