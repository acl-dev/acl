/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"

/* sane_socketpair - sanitize socketpair() error returns */

int acl_sane_socketpair(int domain, int type, int protocol, ACL_SOCKET result[2])
{
	static int socketpair_ok_errors[] = {
		EINTR,
		0,
	};
	int     count;
	int     err;
	int     ret;

	/*
	 * Solaris socketpair() can fail with EINTR.
	 */
	while ((ret = socketpair(domain, type, protocol, result)) < 0) {
		for (count = 0; /* void */ ; count++) {
			if ((err = socketpair_ok_errors[count]) == 0)
				return (ret);
			if (acl_last_error() == err) {
				char tbuf[256];
				acl_msg_warn("socketpair: %s (trying again)",
					acl_last_strerror(tbuf, sizeof(tbuf)));
				sleep(1);
				break;
			}
		}
	}
	return (ret);
}

#elif defined(ACL_WINDOWS)

int acl_sane_socketpair(int domain, int type, int protocol, ACL_SOCKET result[2])
{
	ACL_SOCKET listener = acl_inet_listen("127.0.0.1:0", 1, ACL_BLOCKING);
	char addr[64];

	(void) domain;

	result[0] = ACL_SOCKET_INVALID;
	result[1] = ACL_SOCKET_INVALID;

	if (listener  == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: listen error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return -1;
	}

	acl_tcp_set_nodelay(listener);
	if (acl_getsockname(listener, addr, sizeof(addr)) < 0) {
		acl_msg_error("%s(%d), %s: getoskname error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		acl_socket_close(listener);
		return -1;
	}

	result[0] = acl_inet_connect(addr, ACL_BLOCKING, 0);
	if (result[0] == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: connect %s error %s",
			__FILE__, __LINE__, __FUNCTION__, addr, acl_last_serror());
		acl_socket_close(listener);
		return -1;
	}

	result[1] = acl_inet_accept(listener);

	acl_socket_close(listener);

	if (result[1] == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: accept error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		acl_socket_close(result[0]);
		result[0] = ACL_SOCKET_INVALID;
		return -1;
	}

	acl_tcp_set_nodelay(result[0]);
	acl_tcp_set_nodelay(result[1]);
	return 0;
}

#endif /* ACL_WINDOWS */
