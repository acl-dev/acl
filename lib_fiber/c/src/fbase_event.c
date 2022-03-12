#include "stdafx.h"
#include "common.h"

#if defined(__linux__)
# if defined(ALPINE)
#   define	HAS_EVENTFD
#   include <sys/eventfd.h>
# else
#  include <linux/version.h>
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
#   define	HAS_EVENTFD
#   include <sys/eventfd.h>
#  else
#  endif
# endif
#else
#endif

#include "fiber/libfiber.h"
#include "fiber/fiber_hook.h"
#include "common/iostuff.h"
#include "fiber.h"

void fbase_event_open(FIBER_BASE *fbase)
{
#if defined(HAS_EVENTFD)
	int flags = 0;
# if !defined(ALPINE)
#  if	LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
	flags |= FD_CLOEXEC;
#  endif
# endif
	if (fbase->event_in == INVALID_SOCKET) {
		fbase->event_in  = eventfd(0, flags);
		fbase->event_out = fbase->event_in;
	}
#else
	socket_t fds[2];

	if (fbase->event_in != INVALID_SOCKET) {
		assert(fbase->event_out != INVALID_SOCKET);
		return;
	}

	if (sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds) < 0) {
		msg_fatal("%s(%d), %s: acl_duplex_pipe error %s",
			__FILE__, __LINE__, __FUNCTION__, last_serror());
	}
	fbase->event_in  = fds[0];
	fbase->event_out = fds[1];
#endif

	if (fbase->event_in == INVALID_SOCKET) {
		msg_fatal("%s(%d), %s event_in(%d) invalid",
			__FILE__, __LINE__, __FUNCTION__, (int) fbase->event_in);
	}

	non_blocking(fbase->event_in, 1);
	non_blocking(fbase->event_out, 1);
}

void fbase_event_close(FIBER_BASE *fbase)
{
	if (fbase->event_in != INVALID_SOCKET) {
		CLOSE_SOCKET(fbase->event_in);
	}
	if (fbase->event_out != fbase->event_in
		 && fbase->event_out != INVALID_SOCKET) {
		CLOSE_SOCKET(fbase->event_out);
	}
	fbase->event_in  = INVALID_SOCKET;
	fbase->event_out = INVALID_SOCKET;
}

int fbase_event_wait(FIBER_BASE *fbase)
{
	long long n;
	int  ret, interrupt = 0, err;

	if (fbase->event_in == INVALID_SOCKET) {
		msg_fatal("%s(%d), %s: invalid event_in=%d",
			__FILE__, __LINE__, __FUNCTION__, (int) fbase->event_in);
	}

	while (1) {
#ifdef SYS_WIN
		ret = (int) acl_fiber_recv(fbase->event_in, (char*) &n, sizeof(n), 0);
#else
		ret = (int) acl_fiber_read(fbase->event_in, &n, sizeof(n));
#endif
		if (ret == sizeof(n)) {
			break;
		}

		if (ret >= 0) {
			msg_fatal("%s(%d), %s: read ret=%d invalid length, "
				"interrupt=%d, fd=%d", __FILE__, __LINE__,
				__FUNCTION__, ret, interrupt,
				(int) fbase->event_in);
		}

		err = acl_fiber_last_error();
		if (err == EINTR) {
			interrupt++;
			msg_info("%s(%d), %s: read EINTR=%d, in=%d, ret=%d",
				__FILE__, __LINE__, __FUNCTION__,
				interrupt, (int) fbase->event_in, ret);
			doze(1);
		} else if (err == FIBER_EAGAIN) {
			msg_info("%s(%d), %s: read EAGAIN, in=%d, ret=%d",
				__FILE__, __LINE__, __FUNCTION__,
				(int) fbase->event_in, ret);
			doze(1);
		} else {
			msg_error("%s(%d), %s: read error %s, %d, in=%d, ret=%d,"
				" interrupt=%d", __FILE__, __LINE__,
				__FUNCTION__, last_serror(), err,
				(int) fbase->event_in, ret, interrupt);
			return -1;
		}
	}

	/**
	 * if (atomic_int64_cas(fbase->atomic, 1, 0) != 1) {
	 *	msg_fatal("%s(%d), %s: atomic corrupt",
	 *	__FILE__, __LINE__, __FUNCTION__);
	 * }
	 */
	return 0;
}

int fbase_event_wakeup(FIBER_BASE *fbase)
{
	long long n = 1;
	int  ret, interrupt = 0;

	/**
	 * if (LIKELY(atomic_int64_cas(fbase->atomic, 0, 1) != 0)) {
	 * 	return 0;
	 * }
	 */

	if (fbase->event_out == INVALID_SOCKET) {
		msg_fatal("%s(%d), %s: fbase=%p, invalid event_out=%d",
			__FILE__, __LINE__, __FUNCTION__,
			fbase, (int) fbase->event_out);
	}

	while (1) {
#ifdef SYS_WIN
		ret = (int) acl_fiber_send(fbase->event_out, (char*) &n, sizeof(n), 0);
#else
		ret = (int) acl_fiber_write(fbase->event_out, &n, sizeof(n));
#endif

		if (ret == sizeof(n)) {
			break;
		}

		if (ret >= 0) {
			msg_fatal("%s(%d), %s: write ret=%d invalid length, "
				"interrupt=%d", __FILE__, __LINE__,
				__FUNCTION__, ret, interrupt);
		}

		if (acl_fiber_last_error() == EINTR) {
			interrupt++;
			msg_info("%s(%d), %s: write EINTR=%d, out=%d, ret=%d",
				__FILE__, __LINE__, __FUNCTION__,
				interrupt, (int) fbase->event_out, ret);
			doze(1);
			continue;
		}

		msg_error("%s(%d), %s: write error %s, out=%d, ret=%d, "
			"interrupt=%d", __FILE__, __LINE__, __FUNCTION__,
			last_serror(), (int) fbase->event_out, ret, interrupt);
		return -1;
	}

	return 0;
}

