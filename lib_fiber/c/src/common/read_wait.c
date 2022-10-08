#include "stdafx.h"
#include "fiber/fiber_define.h"
#include "fiber.h"
#include "msg.h"
#include "iostuff.h"

int read_wait(socket_t fd, int delay)
{
	struct pollfd fds;

	fds.events = POLLIN;
	fds.fd = fd;

	for (;;) {
		switch (acl_fiber_poll(&fds, 1, delay)) {
#ifdef SYS_WIN
		case SOCKET_ERROR:
#else
		case -1:
#endif
			if (acl_fiber_last_error() == FIBER_EINTR) {
				continue;
			}
			return -1;
		case 0:
			acl_fiber_set_error(FIBER_ETIME);
			return -1;
		default:
			if ((fds.revents & POLLIN)) {
				return 0;
			}
			if (fds.revents & (POLLHUP | POLLERR | POLLNVAL)) {
				return 0;
			}

			return -1;
		}
	}
}

static int readable(socket_t fd)
{
	struct pollfd fds;
	int    delay = 0;

	fds.fd = fd;
#ifdef SYS_WINDOWS
	fds.events = POLLIN /* | POLLHUP | POLLERR */;
#else
	fds.events = POLLIN | POLLHUP | POLLERR | POLLPRI;
#endif
	fds.revents = 0;

	for (;;) {
		switch (acl_fiber_poll(&fds, 1, delay)) {
#ifdef SYS_WINDOWS
		case SOCKET_ERROR:
#else
		case -1:
#endif
			if (acl_fiber_last_error() == FIBER_EINTR) {
				continue;
			}

			return -1;
		case 0:
			return 0;
		default:
			if ((fds.revents & POLLIN)) {
				return 1;
			} else if (fds.revents & (POLLHUP | POLLERR)) {
				return 1;
			} else {
				return 0;
			}
		}
	}
}

int socket_alive(socket_t fd)
{
	char  buf[16];
	int   ret = readable(fd);

	if (ret == -1) {
		return 0;
	}

	if (ret == 0) {
		return 1;
	}

	ret = (int) acl_fiber_recv(fd, buf, sizeof(buf), MSG_PEEK);

	if (ret == 0) {
		return 0;
	}

	if (ret < 0 && acl_fiber_last_error() != FIBER_EWOULDBLOCK) {
		return 0;
	}

	return 1;
}
