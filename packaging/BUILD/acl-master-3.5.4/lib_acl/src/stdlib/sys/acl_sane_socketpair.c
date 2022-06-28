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
			if ((err = socketpair_ok_errors[count]) == 0) {
				return (ret);
			}
			if (acl_last_error() == err) {
				acl_msg_warn("socketpair: %s (trying again)",
					acl_last_serror());
				sleep(1);
				break;
			}
		}
	}
	return ret;
}

#elif defined(ACL_WINDOWS)

# if defined(ACL_HAS_POLL)

static int check(ACL_SOCKET listener, ACL_SOCKET client, ACL_SOCKET result[2])
{
	int ret;
	struct pollfd fds[2];

	while (result[0] == ACL_SOCKET_INVALID || result[1] ==ACL_SOCKET_INVALID) {
		int i = 0;
		if (result[1] == ACL_SOCKET_INVALID) {
			fds[i].fd      = listener;
			fds[i].events  = POLLIN;
			fds[i].revents = 0;
			i++;
		}

		if (result[0] == ACL_SOCKET_INVALID) {
			fds[i].fd      = client;
			fds[i].events  = POLLOUT;
			fds[i].revents = 0;
		}

		ret = WSAPoll(fds, 2, 10000);
		if (ret < 0) {
			if (acl_last_error() == ACL_EINTR) {
				continue;
			}
			acl_msg_error("WSAPoll error: %s", acl_last_serror());
			return -1;
		} else if (ret == 0) {
			acl_msg_error("WSAPoll timeout: %s", acl_last_serror());
			return -1;
		}

		if ((fds[0].revents & POLLIN)) {
			result[1] = accept(listener, NULL, 0);
		}
		if ((fds[1].revents & POLLOUT)) {
			result[0] = client;
		}
	}

	return 0;
}

# else

static int check(ACL_SOCKET listener, ACL_SOCKET client, ACL_SOCKET result[2])
{
	int ret;
	struct timeval tv;
	fd_set rmask, wmask, xmask;

	while (result[0] == ACL_SOCKET_INVALID || result[1] ==ACL_SOCKET_INVALID) {
		FD_ZERO(&rmask);
		FD_ZERO(&wmask);
		FD_ZERO(&xmask);
		tv.tv_usec = 10;
		tv.tv_sec  = 0;

		if (result[1] == ACL_SOCKET_INVALID) {
			FD_SET(listener, &rmask);
			FD_SET(listener, &xmask);
		}

		if (result[0] == ACL_SOCKET_INVALID) {
			FD_SET(client, &wmask);
			FD_SET(client, &xmask);
		}

		ret = select(2, &rmask, &wmask, &xmask, NULL);
		if (ret == 0) {
			acl_msg_error("select timeout: %s", acl_last_serror());
			return -1;
		} else if (ret < 0) {
			if (acl_last_error() == ACL_EINTR) {
				continue;
			}
			acl_msg_error("select error: %s", acl_last_serror());
			return -1;
		}

		if (FD_ISSET(listener, &xmask)) {
			acl_msg_error("listener exception");
			return -1;
		}

		if (FD_ISSET(client, &xmask)) {
			acl_msg_error("client exception");
			return -1;
		}

		if (FD_ISSET(listener, &rmask)) {
			result[1] = accept(listener, NULL, 0);
		}

		if (FD_ISSET(client, &wmask)) {
			result[0] = client;
		}
	}

	return 0;
}

# endif /* ACL_HAS_POLL */

int acl_sane_socketpair(int domain, int type, int protocol, ACL_SOCKET result[2])
{
	ACL_SOCKET listener = acl_inet_listen("127.0.0.1:0", 10, 0), client;
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

	client = acl_inet_connect(addr, ACL_NON_BLOCKING, 0);
	if (client == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: connect %s error %s",
			__FILE__, __LINE__, __FUNCTION__, addr, acl_last_serror());
		acl_socket_close(listener);
		return -1;
	}

	acl_non_blocking(client, ACL_BLOCKING);

	if (check(listener, client, result) == -1) {
		acl_socket_close(listener);
		return -1;
	}

	acl_socket_close(listener);
	acl_tcp_set_nodelay(result[0]);
	acl_tcp_set_nodelay(result[1]);
	acl_tcp_so_linger(result[0], 1, 0);
	acl_tcp_so_linger(result[1], 1, 0);
	return 0;
}

#endif /* ACL_WINDOWS */
