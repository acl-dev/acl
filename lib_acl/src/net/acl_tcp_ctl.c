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

#include "stdlib/acl_msg.h"
#include "net/acl_tcp_ctl.h"

#endif

void acl_tcp_set_rcvbuf(ACL_SOCKET fd, int size)
{
	const char *myname = "acl_tcp_set_rcvbuf";
	char  ebuf[256];

	if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &size, sizeof(size)) < 0)
		acl_msg_error("%s(%d): size(%d), setsockopt error(%s)",
				myname, __LINE__, size, acl_last_strerror(ebuf, sizeof(ebuf)));
}

void acl_tcp_set_sndbuf(ACL_SOCKET fd, int size)
{
	const char *myname = "acl_tcp_sndbuf";
	char  ebuf[256];

	if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, sizeof(size)) < 0)
		acl_msg_error("%s: FD %d, SIZE %d: %s\n",
				myname, fd, size, acl_last_strerror(ebuf, sizeof(ebuf)));
}

int acl_tcp_get_rcvbuf(ACL_SOCKET fd)
{
	const char *myname = "acl_tcp_get_rcvbuf";
	char  ebuf[256];
	int   size;
	socklen_t len;

	len = (socklen_t) sizeof(size);
	if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char *) &size, &len) < 0) {
		acl_msg_error("%s(%d): size(%d), setsockopt error(%s)",
			myname, __LINE__, size, acl_last_strerror(ebuf, sizeof(ebuf)));
		return (-1);
	}

	return (size);
}

int acl_tcp_get_sndbuf(ACL_SOCKET fd)
{
	const char *myname = "acl_tcp_get_sndbuf";
	char  ebuf[256];
	int   size;
	socklen_t len;

	len = (socklen_t) sizeof(size);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *) &size, &len) < 0) {
		acl_msg_error("%s(%d): size(%d), setsockopt error(%s)",
			myname, __LINE__, size, acl_last_strerror(ebuf, sizeof(ebuf)));
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

	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *) &on, sizeof(on)) < 0) {
		char  ebuf[256];
		acl_msg_error("%s(%d): set nodelay error(%s), onoff(%d)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)), onoff);
	}
}

void acl_tcp_defer_accept(ACL_SOCKET fd acl_unused, int timeout acl_unused)
{
#ifdef	TCP_DEFER_ACCEPT
	const char *myname = "acl_tcp_defer_accept";
	char  ebuf[256];

	if (timeout < 0)
		timeout = 0;
	if (setsockopt(fd, IPPROTO_TCP, TCP_DEFER_ACCEPT, &timeout, sizeof(timeout)) < 0)
		acl_msg_error("%s: setsockopt(TCP_DEFER_ACCEPT): %s",
			myname, acl_last_strerror(ebuf, sizeof(ebuf)));
#endif
}

void acl_tcp_so_linger(ACL_SOCKET fd, int onoff, int timeout)
{
	const char *myname = "acl_tcp_so_linger";
	struct linger  l;

	l.l_onoff = onoff ? 1 : 0;
	l.l_linger = timeout >= 0 ? timeout : 0;
	if (setsockopt(fd, SOL_SOCKET, SO_LINGER, (char *) &l, sizeof(l)) < 0) {
		char  ebuf[256];
		acl_msg_error("%s(%d): setsockopt(SO_LINGER) error(%s), onoff(%d), timeout(%d)",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)), onoff, timeout);
	}
}
