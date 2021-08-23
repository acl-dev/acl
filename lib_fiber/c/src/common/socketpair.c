#include "stdafx.h"
#include "fiber/libfiber.h"
#include "msg.h"
#include "iostuff.h"

#ifdef SYS_WIN

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

int sane_socketpair(int domain, int type, int protocol, socket_t result[2])
{
	socket_t listener = inet_listen("127.0.0.1", 0, 10);
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

	result[0] = socket(AF_INET, SOCK_STREAM, 0);
	if (result[0] == INVALID_SOCKET) {
		msg_error("%s(%d), %s: create socket %s error %s",
			__FILE__, __LINE__, __FUNCTION__, addr, last_serror());
		CLOSE_SOCKET(listener);
		return -1;
	}
	if (connect(result[0], sa, len) == -1) {
		msg_error("%s(%d), %s: connect error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		CLOSE_SOCKET(listener);
		CLOSE_SOCKET(result[0]);
		result[0] = INVALID_SOCKET;
		return -1;
	}

	result[1] = accept(listener, NULL, 0);
	CLOSE_SOCKET(listener);

	if (result[1] == INVALID_SOCKET) {
		msg_error("%s(%d), %s: accept error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
		CLOSE_SOCKET(result[0]);
		result[0] = INVALID_SOCKET;
		return -1;
	}

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
