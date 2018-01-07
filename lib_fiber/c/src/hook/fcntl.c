#include "stdafx.h"
#include <stdarg.h>
#include "common.h"
#include "fiber.h"
#include "hook.h"

#if defined(__linux__) || defined(__FreeBSD__)

int fcntl(int fd, int cmd, ...)
{
	int     arg_int, ret = 0;
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
#ifdef	__linux__
		if (!var_hook_sys_api || (ret = epoll_try_register(fd)) == -1) {
			arg_int = va_arg(ap, int);
			ret = (*sys_fcntl)(fd, cmd, arg_int);
		}
#else
		if (!var_hook_sys_api) {
			arg_int = va_arg(ap, int);
			ret = (*sys_fcntl)(fd, cmd, arg_int);
		}
#endif
		break;
#ifdef	F_DUPFD_CLOEXEC
	case F_DUPFD_CLOEXEC:
#ifdef	__linux__
		if (!var_hook_sys_api || (ret = epoll_try_register(fd)) == -1) {
			arg_int = va_arg(ap, int);
			ret = (*sys_fcntl)(fd, cmd, arg_int);
		} else {
			close_on_exec(fd, 1);
		}
#else
		if (!var_hook_sys_api) {
			arg_int = va_arg(ap, int);
			ret = (*sys_fcntl)(fd, cmd, arg_int);
		} else {
			close_on_exec(fd, 1);
		}
#endif
		break;
#endif
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
#ifdef	F_GETSIG
	case F_GETSIG:
#endif
#ifdef	F_GETLEASE
	case F_GETLEASE:
#endif
#ifdef	F_GETPIPE_SZ
	case F_GETPIPE_SZ:
#endif
		ret = (*sys_fcntl)(fd, cmd);
		break;
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
#ifdef	F_SETSIG
	case F_SETSIG:
#endif
#ifdef	F_SETLEASE
	case F_SETLEASE:
#endif
#ifdef	F_NOTIFY
	case F_NOTIFY:
#endif
#ifdef	F_GET_RW_HINT
	case F_GET_RW_HINT:
#endif
#ifdef	F_SET_RW_HINT
	case F_SET_RW_HINT:
#endif
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
