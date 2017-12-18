#include "stdafx.h"
#include <fcntl.h>
#include <string.h>

/* Utility library. */

#include "msg.h"
#include "iostuff.h"

#define PATTERN	FD_CLOEXEC

/* close_on_exec - set/clear close-on-exec flag */

int close_on_exec(int fd, int on)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFD, 0)) < 0) {
		msg_fatal("fcntl: get flags: %s", last_serror());
	}
	if (fcntl(fd, F_SETFD, on ? flags | PATTERN : flags & ~PATTERN) < 0) {
		msg_fatal("fcntl: set close-on-exec flag %s: %s",
			on ? "on" : "off", last_serror());
	}

	return ((flags & PATTERN) != 0);
}
