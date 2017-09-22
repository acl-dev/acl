#include "stdafx.h"
#include <fcntl.h>

#include "msg.h"
#include "iostuff.h"

/* Backwards compatibility */
# ifndef O_NONBLOCK
#  define PATTERN	FNDELAY
# else
#  define PATTERN	O_NONBLOCK
# endif

int non_blocking(int fd, int on)
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

	return flags;
}
