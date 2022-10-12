#include "stdafx.h"
#include "common.h"

#include "fiber.h"
#include "hook.h"

#ifdef	HAS_IO_URING
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../event/event_io_uring.h"

static void file_open_callback(EVENT *ev UNUSED, FILE_EVENT *fe)
{
	if (fe->fiber_r->status != FIBER_STATUS_READY) {
		acl_fiber_ready(fe->fiber_r);
	}
}

int open(const char *pathname, int flags, ...)
{
	FILE_EVENT *fe;
	EVENT *ev;
	va_list ap;
	mode_t mode;

	va_start(ap, flags);
	mode = va_arg(ap, mode_t);
	va_end(ap);

	if (sys_open == NULL) {
		hook_once();
		if (sys_open == NULL) {
			msg_error("%s: sys_open NULL", __FUNCTION__);
			return -1;
		}
	}

	if (!var_hook_sys_api) {
		return (*sys_open)(pathname, flags, mode);
	}

	fiber_io_check();

	ev = fiber_io_event();

	if (!EVENT_IS_IO_URING(ev)) {
		return (*sys_open)(pathname, flags, mode);
	}

	fe = file_event_alloc(-1);
	fe->fiber_r = acl_fiber_running();
	fe->fiber_w = acl_fiber_running();

	fe->fiber_r->status = FIBER_STATUS_NONE;
	fe->mask = EVENT_FILE_OPEN;
	fe->r_proc = file_open_callback;
	fe->rbuf = strdup(pathname);
	event_uring_file_open(ev, fe, fe->rbuf, flags, mode);

	fiber_io_inc();
	acl_fiber_switch();
	fiber_io_dec();

	fe->mask &= ~EVENT_FILE_OPEN;
	free(fe->rbuf);
	fe->rbuf = NULL;

	if (fe->fd >= 0) {
		fiber_file_set(fe);
		fe->type = TYPE_FILE | TYPE_EVENTABLE;
		return fe->fd;
	}

	acl_fiber_set_error(-fe->fd);
	file_event_unrefer(fe);
	return -1;
}

#endif // HAS_IO_URING

