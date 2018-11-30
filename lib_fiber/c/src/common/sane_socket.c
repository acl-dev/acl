#include "stdafx.h"
#include "fiber/libfiber.h"
#include "sane_socket.h"

int is_listen_socket(socket_t fd)
{
	int val, ret;
#ifdef SYS_WIN
	int len = sizeof(val);
#else
	socklen_t len = sizeof(val);
#endif

	ret = getsockopt(fd, SOL_SOCKET, SO_ACCEPTCONN, (char*) &val, &len);
	if (ret == -1) {
		return 0;
	} else if (val) {
		return 1;
	} else {
		return 0;
	}
}

int getsocktype(socket_t fd)
{
	SOCK_ADDR addr;
	struct sockaddr *sa = (struct sockaddr*) &addr;
	socklen_t len = sizeof(addr);

	if (fd == INVALID_SOCKET) {
		return -1;
	}

	if (getsockname(fd, sa, &len) == -1) {
		return -1;
	}

#ifndef	SYS_WIN
	if (sa->sa_family == AF_UNIX) {
		return AF_UNIX;
	}
#endif
#ifdef AF_INET6
	if (sa->sa_family == AF_INET || sa->sa_family == AF_INET6) {
#else
	if (sa->sa_family == AF_INET) {
#endif
		return sa->sa_family;
	}

	return -1;
}
