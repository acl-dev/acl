#include "stdafx.h"
#include "fiber/fiber_base.h"

#include "msg.h"
#include "iostuff.h"

#ifdef SYS_WIN

int non_blocking(socket_t fd, int on)
{
	unsigned long n = on;
	int flags = 0;

	if (ioctlsocket(fd, FIONBIO, &n) < 0) {
		msg_error("ioctlsocket(fd,FIONBIO) failed");
		return -1;
	}
	return flags;
}

static __thread __is_non_blocking = 0;

void acl_fiber_set_non_blocking(int yes)
{
	__is_non_blocking = yes;
}

int is_non_blocking(socket_t fd)
{
	return __is_non_blocking;
}


#elif defined(SYS_UNIX)

# ifndef O_NONBLOCK
#  define PATTERN	FNDELAY
# else
#  define PATTERN	O_NONBLOCK
# endif

int non_blocking(socket_t fd, int on)
{
	int   flags;
	int   nonb = PATTERN;

	/*
	** NOTE: consult ALL your relevant manual pages *BEFORE* changing
	**	 these ioctl's.  There are quite a few variations on them,
	**	 as can be seen by the PCS one.  They are *NOT* all the same.
	**	 Heed this well. - Avalon.
	*/
#ifdef	NBLOCK_POSIX
	nonb |= O_NONBLOCK;
#endif
#ifdef	NBLOCK_BSD
	nonb |= O_NDELAY;
#endif

	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		msg_error("%s(%d), %s: fcntl(%d, F_GETFL) error: %s",
			__FILE__, __LINE__, __FUNCTION__,
			fd, last_serror());
		return -1;
	}
	if (fcntl(fd, F_SETFL, on ? flags | nonb : flags & ~nonb) < 0) {
		msg_error("%s(%d), %s: fcntl(%d, F_SETL, nonb) error: %s",
			__FILE__, __LINE__, __FUNCTION__,
			fd, last_serror());
		return -1;
	}

	return (flags & PATTERN) ? 1 : 0;
}

int is_non_blocking(socket_t fd)
{
	int flags;

	if ((flags = fcntl(fd, F_GETFL)) == -1) {
		msg_error("%s(%d), %s: fcntl(%d, F_GETFL) error: %s",
			__FILE__, __LINE__, __FUNCTION__,
			fd, last_serror());
		return 0;
	}

	return (flags & PATTERN) ? 1 : 0;
}

void acl_fiber_set_non_blocking(int yes)
{
	(void) yes;
}

#endif
