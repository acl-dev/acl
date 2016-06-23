#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "net/acl_tcp_ctl.h"
#include "net/acl_connect.h"

#endif

/* acl_sane_connect - sanitize connect() results */

int acl_sane_connect(ACL_SOCKET sock, const struct sockaddr * sa, socklen_t len)
{
	int   on;

	/*
	 * XXX Solaris select() produces false read events, so that read()
	 * blocks forever on a blocking socket, and fails with EAGAIN on
	 * a non-blocking socket. Turning on keepalives will fix a blocking
	 * socket provided that the kernel's keepalive timer expires before
	 * the Postfix watchdog timer.
	 */

	if (sa->sa_family == AF_INET) {
		/* default set to nodelay --- zsx, 2008.9.4*/
		acl_tcp_nodelay(sock, 1);
#if defined(BROKEN_READ_SELECT_ON_TCP_SOCKET) && defined(SO_KEEPALIVE)
		on = 1;
		(void) setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE,
				  (char *) &on, sizeof(on));
#endif
	}

	on = 1;

	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		(char *) &on, sizeof(on)) < 0)
	{
		acl_msg_error("acl_sane_connect: setsockopt error(%s)",
			acl_last_serror());
	}

	return connect(sock, sa, len);
}

