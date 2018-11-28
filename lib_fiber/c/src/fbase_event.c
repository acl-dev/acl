#include "stdafx.h"
#include "common.h"

#ifdef SYS_UNIX

#if defined(__linux__)
# include <linux/version.h>
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#  define	HAS_EVENTFD
#  include <sys/eventfd.h>
# else
# endif
#else
#endif

#include "fiber/libfiber.h"
#include "fiber.h"

void fbase_event_open(FIBER_BASE *fbase)
{
#if defined(HAS_EVENTFD)
	int flags = 0;
# if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	flags |= FD_CLOEXEC;
# endif
	flags = 0;
	if (fbase->event_in == -1) {
		fbase->event_in  = eventfd(0, flags);
		fbase->event_out = fbase->event_in;
	}
#else
	int fds[2];

	if (fbase->event_in >= 0) {
		assert(fbase->event_out >= 0);
		return;
	}

	if (sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
		msg_fatal("%s(%d), %s: acl_duplex_pipe error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
	fbase->event_in  = fds[0];
	fbase->event_out = fds[1];
#endif
}

void fbase_event_close(FIBER_BASE *fbase)
{
	if (fbase->event_in >= 0) {
		close(fbase->event_in);
	}
	if (fbase->event_out != fbase->event_in && fbase->event_out >= 0) {
		close(fbase->event_out);
	}
	fbase->event_in  = -1;
	fbase->event_out = -2;
	//atomic_int64_set(fbase->atomic, 0);
}

int fbase_event_wait(FIBER_BASE *fbase)
{
	long long n;

	assert(fbase->event_in >= 0);
	if (read(fbase->event_in, &n, sizeof(n)) != sizeof(n)) {
		msg_error("%s(%d), %s: read error %s, in=%d",
			__FILE__, __LINE__, __FUNCTION__,
			last_serror(), fbase->event_in);
		return -1;
	}
	/*
	if (atomic_int64_cas(fbase->atomic, 1, 0) != 1) {
		msg_fatal("%s(%d), %s: atomic corrupt",
			__FILE__, __LINE__, __FUNCTION__);
	}
	*/
	return 0;
}

int fbase_event_wakeup(FIBER_BASE *fbase)
{
	long long n = 1;

	/*
	if (LIKELY(atomic_int64_cas(fbase->atomic, 0, 1) != 0)) {
		return 0;
	}
	*/

	assert(fbase->event_out >= 0);
	if (write(fbase->event_out, &n, sizeof(n)) != sizeof(n)) {
		msg_error("%s(%d), %s: write error %s, out=%d",
			__FILE__, __LINE__, __FUNCTION__,
			last_serror(), fbase->event_out);
		return -1;
	}

	return 0;
}

#endif // SYS_UNIX
