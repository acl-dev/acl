#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef	ACL_UNIX
#include <fcntl.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"

#endif

#ifdef	ACL_UNIX
/* Backwards compatibility */
# ifndef O_NONBLOCK
#  define PATTERN	FNDELAY
# else
#  define PATTERN	O_NONBLOCK
# endif
#endif

int acl_non_blocking(ACL_SOCKET fd, int on)
{
#ifdef	ACL_UNIX
	int   flags;
	int   nonb = PATTERN;
#endif
#ifdef	NBLOCK_SYSV
	int	res;
#endif

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

#ifdef	NBLOCK_SYSV
	/* This portion of code might also apply to NeXT.  -LynX */
	if (on)
		res = 1;
	else
		res = 0;
	if (ioctl (fd, FIONBIO, &res) < 0) {
		acl_msg_error("ioctl(fd,FIONBIO) failed");
		return (-1);
	}
#elif defined(ACL_UNIX)
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		acl_msg_error("fcntl(fd, F_GETFL) failed");
		return (-1);
	}
	if (fcntl(fd, F_SETFL, on ? flags | nonb : flags & ~nonb) < 0) {
		acl_msg_error("fcntl(fd, F_SETL, nonb) failed");
		return (-1);
	}
#elif defined(WIN32)
	if (ioctlsocket(fd, FIONBIO, (unsigned long *) &on) < 0) {
		acl_msg_error("ioctlsocket(fd,FIONBIO) failed");
		return (-1);
	}
#else
# error "unknown OS type"
#endif
	return (0);
}
