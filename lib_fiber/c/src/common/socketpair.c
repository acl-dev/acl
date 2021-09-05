#include "stdafx.h"
#include "fiber/libfiber.h"
#include "msg.h"
#include "iostuff.h"

#ifdef SYS_WIN

#include "../hook/hook.h"
#include "../fiber.h"

static socket_t inet_listen(const char *addr, int port, int backlog)
{
	socket_t s;
	struct sockaddr_in in;

	in.sin_addr.s_addr = inet_addr(addr);
	in.sin_port        = htons(port);
	in.sin_family      = AF_INET;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == INVALID_SOCKET) {
		msg_error("%s(%d), %s: create listen socket error=%s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		return INVALID_SOCKET;
	}

	if (bind(s, (const struct sockaddr*)&in, sizeof(in)) < 0) {
		msg_error("%s(%d), %s: bind %s error %s",
			__FILE__, __LINE__, __FUNCTION__, addr, last_serror());
		closesocket(s);
		return INVALID_SOCKET;
	}

	if (listen(s, backlog) < 0) {
		msg_error("%s(%d), %s: listen %s error %s",
			__FILE__, __LINE__, __FUNCTION__, addr, last_serror());
		closesocket(s);
		return INVALID_SOCKET;
	}
	return s;
}

static int check(socket_t listener, socket_t client, socket_t result[2])
{
	int ret;
	struct pollfd fds[2];

	memset(fds, 0, sizeof(fds));

	while (result[0] == INVALID_SOCKET || result[1] ==INVALID_SOCKET) {
		int i = 0;
		if (result[1] == INVALID_SOCKET) {
			fds[i].fd      = listener;
			fds[i].events  = POLLIN;
			fds[i].revents = 0;
			i++;
		}

		if (result[0] == INVALID_SOCKET) {
			fds[i].fd      = client;
			fds[i].events  = POLLOUT;
			fds[i].revents = 0;
		}

		if (var_hook_sys_api) {
			ret = (*sys_poll)(fds, 2, -1);
		} else {
			ret = WSAPoll(fds, 2, -1);
		}

		if (ret <= 0) {
			msg_error("WSAPoll error: %s", last_serror());
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

int sane_socketpair(int domain, int type, int protocol, socket_t result[2])
{
	socket_t listener = inet_listen("127.0.0.1", 0, 10), client;
	struct sockaddr_in addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (listener == INVALID_SOCKET) {
		return -1;
	}

	result[0] = INVALID_SOCKET;
	result[1] = INVALID_SOCKET;

	if (listener  == INVALID_SOCKET) {
		msg_error("%s(%d), %s: listen error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		return -1;
	}

	tcp_nodelay(listener, 1);
	if (getsockname(listener, sa, &len) < 0) {
		msg_error("%s(%d), %s: getoskname error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		CLOSE_SOCKET(listener);
		return -1;
	}

	client = socket(AF_INET, SOCK_STREAM, 0);
	if (client == INVALID_SOCKET) {
		msg_error("%s(%d), %s: create socket %s error %s",
			__FILE__, __LINE__, __FUNCTION__, addr, last_serror());
		CLOSE_SOCKET(listener);
		return -1;
	}

	non_blocking(client, NON_BLOCKING);

	if (!var_hook_sys_api) {
		if (connect(client, sa, len) == -1) {
			int err = acl_fiber_last_error();
			if (err != FIBER_EINPROGRESS && !error_again(err)) {
				msg_error("%s(%d), %s: connect error %s",
					__FILE__, __LINE__, __FUNCTION__, last_serror());
				CLOSE_SOCKET(listener);
				CLOSE_SOCKET(client);
				return -1;
			}
		}
	} else if ((*sys_connect)(client, sa, len) == -1) {
		int err = acl_fiber_last_error();
		if (err != FIBER_EINPROGRESS && !error_again(err)) {
			msg_error("%s(%d), %s: connect error %s",
				__FILE__, __LINE__, __FUNCTION__, last_serror());
			CLOSE_SOCKET(listener);
			CLOSE_SOCKET(client);
			return -1;
		}
	}

	non_blocking(client, BLOCKING);

	if (check(listener, client, result) == -1) {
		CLOSE_SOCKET(listener);
		return -1;
	}

	CLOSE_SOCKET(listener);
	tcp_nodelay(result[0], 1);
	tcp_nodelay(result[1], 1);
	return 0;
}

#elif defined(SYS_UNIX)

/* sane_socketpair - sanitize socketpair() error returns */

int sane_socketpair(int domain, int type, int protocol, int result[2])
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
				return ret;
			if (acl_fiber_last_error() == err) {
				msg_warn("socketpair: %s (trying again)",
					last_serror());
				sleep(1);
				break;
			}
		}
	}
	return ret;
}

#endif
