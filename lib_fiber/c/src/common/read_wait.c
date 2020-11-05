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
			acl_fiber_set_error(FIBER_ETIMEDOUT);
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
