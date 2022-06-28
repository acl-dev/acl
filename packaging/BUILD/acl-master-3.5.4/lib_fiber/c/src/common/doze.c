#include "stdafx.h"
#include "fiber/libfiber.h"
#include "msg.h"
#include "iostuff.h"

/* doze - sleep a while */

#ifdef SYS_WIN
void doze(unsigned delay) {
    Sleep(delay);
}
#else
void doze(unsigned delay)
{
	struct timeval tv;

	tv.tv_sec = delay / 1000;
	tv.tv_usec = (suseconds_t) (delay - tv.tv_sec * 1000) * 1000;

	while (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv) < 0) {
		if (acl_fiber_last_error() != FIBER_EINTR) {
			msg_fatal("doze: select: %s", last_serror());
		}
	}
}
#endif
