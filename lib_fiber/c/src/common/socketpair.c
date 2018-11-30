#include "stdafx.h"
#include "fiber/libfiber.h"
#include "msg.h"
#include "iostuff.h"

#ifdef SYS_UNIX

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
