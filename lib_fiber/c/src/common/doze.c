#include "stdafx.h"
#include <errno.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>

#include "msg.h"
#include "iostuff.h"

/* doze - sleep a while */

void doze(unsigned delay)
{
	struct timeval tv;

	tv.tv_sec = delay / 1000;
	tv.tv_usec = (suseconds_t) (delay - tv.tv_sec * 1000) * 1000;

	while (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv) < 0) {
		if (last_error() != EINTR) {
			msg_fatal("doze: select: %s", last_serror());
		}
	}
}
