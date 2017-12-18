#include "stdafx.h"
#include "msg.h"
#include "iostuff.h"

static int read_poll_wait(int fd, int delay)
{
	const char *myname = "read_poll_wait";
	struct pollfd fds;
	time_t begin;

	fds.events = POLLIN | POLLHUP | POLLERR;
	fds.fd = fd;

	set_error(0);

	for (;;) {
		time(&begin);

		switch (poll(&fds, 1, delay)) {
		case -1:
			if (last_error() == EINTR)
				continue;

			msg_error("%s(%d), %s: poll error(%s), fd: %d",
				__FILE__, __LINE__, myname,
				last_serror(), (int) fd);
			return -1;
		case 0:
			set_error(ETIMEDOUT);
			return -1;
		default:
			if ((fds.revents & POLLIN))
				return 0;
			else if (fds.revents & (POLLHUP | POLLERR)) {
				msg_warn("%s(%d), %s: poll error: %s, "
					"fd: %d, delay: %d, spent: %ld",
					__FILE__, __LINE__, myname,
					last_serror(), fd, delay,
					(long) (time(NULL) - begin));
				return -1;
			} else {
				msg_warn("%s(%d), %s: poll error: %s, "
					"fd: %d, delay: %d, spent: %ld",
					__FILE__, __LINE__, myname,
					last_serror(), fd, delay,
					(long) (time(NULL) - begin));
				return -1;
			}
		}
	}
}

int read_wait(int fd, int timeout)
{
	return read_poll_wait(fd, timeout * 1000);
}
