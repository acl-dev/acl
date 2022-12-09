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
#include "hook/hook.h"
#include "hook/io.h"
#include "fiber.h"

//#undef	HAS_EVENTFD

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

	fbase->in = fiber_file_open_read(fbase->event_in);
	if (fbase->event_in == fbase->event_out) {
		fbase->out = fbase->in;
	} else {
		fbase->out = fiber_file_open_write(fbase->event_out);
	}

#if defined(HAS_IO_URING)
	non_blocking(fbase->event_in, 0);
	non_blocking(fbase->event_out, 0);

	fbase->in->type = TYPE_SPIPE | TYPE_EVENTABLE;
	fbase->in->mask = EVENT_SYSIO;

	if (fbase->out != fbase->in) {
		fbase->out->type = TYPE_SPIPE | TYPE_EVENTABLE;
		fbase->out->mask = EVENT_SYSIO;
	}
#else
	non_blocking(fbase->event_in, 1);
	non_blocking(fbase->event_out, 1);
#endif
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

	// Set the fds to INVALID_SOCKET to avoid closing the same fd
	// more than once, because the fbase_event_close() maybe be called
	// more than once:(
	fbase->event_in  = INVALID_SOCKET;
	fbase->event_out = INVALID_SOCKET;
	fbase->in  = NULL;
	fbase->out = NULL;
}

int fbase_event_wait(FIBER_BASE *fbase)
{
	long long n;
	int  ret, interrupt = 0, err;
	socket_t fd = fbase->in->fd;

	if (fd == INVALID_SOCKET) {
		msg_fatal("%s(%d), %s: invalid event_in=%d",
			__FILE__, __LINE__, __FUNCTION__, (int) fd);
	}


	while (1) {
		if (read_wait(fbase->event_in, -1) == -1) {
			msg_error("%s(%d), %s: read_wait error=%s, fd=%d",
				__FILE__, __LINE__, __FUNCTION__,
				last_serror(), fbase->event_in);
			return -1;
		}

		if (var_hook_sys_api) {
#ifdef	HAS_EVENTFD
			// The eventfd can only use read API.
			ret = (int) fiber_read(fbase->in, (char*) &n, sizeof(n));
#else
			ret = (int) fiber_recv(fbase->in, (char*) &n, sizeof(n), 0);
#endif
		} else {
#ifdef	HAS_EVENTFD
			ret = (int) acl_fiber_read(fd, (char*)& n, sizeof(n));
#else
			ret = (int) acl_fiber_recv(fd, (char*)& n, sizeof(n), 0);
#endif
		}

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
			msg_info("%s(%d), %s: scheduled %s, read EAGAIN, "
				"in=%d, ret=%d", __FILE__, __LINE__,
				__FUNCTION__, acl_fiber_scheduled() ? "yes":"no",
				(int) fbase->event_in, ret);
			doze(1);
		} else {
			msg_error("%s(%d), %s: read error %s, %d, in=%d, ret=%d,"
				" interrupt=%d, fiber=%d", __FILE__, __LINE__,
				__FUNCTION__, last_serror(), err,
				(int) fbase->event_in, ret, interrupt,
				acl_fiber_self());
			return -1;
		}
	}

	return 0;
}

int fbase_event_wakeup(FIBER_BASE *fbase)
{
	long long n = 1;
	int  ret, interrupt = 0;
	socket_t fd = fbase->out->fd;

	if (fd == INVALID_SOCKET) {
		msg_fatal("%s(%d), %s: fbase=%p, invalid event_out=%d",
			__FILE__, __LINE__, __FUNCTION__, fbase, (int) fd);
	}

	while (1) {
		if (var_hook_sys_api) {
#ifdef	HAS_EVENTFD
			// The eventfd can only use write API.
			ret = fiber_write(fbase->out, (char*) &n, sizeof(n));
#else
			ret = fiber_send(fbase->out, (char*) &n, sizeof(n), 0);
#endif
		} else {
#ifdef	HAS_EVENTFD
			ret = acl_fiber_write(fd, (char*) &n, sizeof(n));
#else
			ret = acl_fiber_send(fd, (char*) &n, sizeof(n), 0);
#endif
		}

		if (ret == sizeof(n)) {
			break;
		}

		if (ret >= 0) {
			msg_fatal("%s(%d), %s: fiber=%d, write ret=%d invalid"
				" length, interrupt=%d, fd=%d",
				__FILE__, __LINE__, __FUNCTION__,
				acl_fiber_self(), ret, interrupt,
				(int) fbase->event_out);
		}

		if (acl_fiber_last_error() == EINTR) {
			interrupt++;
			msg_info("%s(%d), %s: write EINTR=%d, out=%d, ret=%d",
				__FILE__, __LINE__, __FUNCTION__,
				interrupt, (int) fbase->event_out, ret);
			doze(1);
			continue;
		}

		msg_error("%s(%d), %s: write error %s, fb=%p, out=%d, ret=%d, "
			"interrupt=%d, fiber=%d", __FILE__, __LINE__,
			__FUNCTION__, last_serror(), fbase,
			(int) fbase->event_out, ret, interrupt,
			acl_fiber_self());
		return -1;
	}

	return 0;
}
