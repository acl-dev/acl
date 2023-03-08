#include "stdafx.h"
#include <stdarg.h>
#include "common.h"
#include "fiber.h"
#include "hook.h"

#if defined(__linux__)

int fcntl(int fd, int cmd, ...)
{
	int     arg_int, ret;
#if	defined(F_GETOWN_EX) || defined(F_SETOWN_EX)
	struct  f_owner_ex * arg_owner;
#endif
	struct  flock *arg_lock;
	va_list ap;

	if (sys_fcntl == NULL) {
		hook_once();
	}

	va_start(ap, cmd);

	switch (cmd) {
	case F_DUPFD:
		if (!var_hook_sys_api || (ret = epoll_try_register(fd)) == -1) {
			arg_int = va_arg(ap, int);
			ret = (*sys_fcntl)(fd, cmd, arg_int);
		}
		break;
#ifdef	F_DUPFD_CLOEXEC
	case F_DUPFD_CLOEXEC:
		if (!var_hook_sys_api || (ret = epoll_try_register(fd)) == -1) {
			arg_int = va_arg(ap, int);
			ret = (*sys_fcntl)(fd, cmd, arg_int);
		} else {
			close_on_exec(fd, 1);
		}
		break;
#endif
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
	case F_GETSIG:
	case F_GETLEASE:
#ifdef	F_GETPIPE_SZ
	case F_GETPIPE_SZ:
#endif
		ret = (*sys_fcntl)(fd, cmd);
		break;
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
	case F_SETSIG:
	case F_SETLEASE:
	case F_NOTIFY:
#ifdef	F_SETPIPE_SZ
	case F_SETPIPE_SZ:
#endif
		arg_int= va_arg(ap, long);
		ret = (*sys_fcntl)(fd, cmd, arg_int);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		arg_lock = va_arg(ap, struct flock*);
		ret = (*sys_fcntl)(fd, cmd, arg_lock);
		break;
#ifdef	F_GETOWN_EX
	case F_GETOWN_EX:
		arg_owner = va_arg(ap, struct f_owner_ex *);
		ret = (*sys_fcntl)(fd, cmd, arg_owner);
		break;
#endif
#ifdef	F_SETOWN_EX
	case F_SETOWN_EX:
		arg_owner = va_arg(ap, struct f_owner_ex *);
		ret = (*sys_fcntl)(fd, cmd, arg_owner);
		break;
#endif
	default:
		ret = -1;
		msg_error("%s(%d), %s: unknown cmd: %d, fd: %d",
			__FILE__, __LINE__, __FUNCTION__, cmd, fd);
		break;
	}

	va_end(ap);

	if (ret < 0) {
		fiber_save_errno(acl_fiber_last_error());
	}

	return ret;
}

#endif
