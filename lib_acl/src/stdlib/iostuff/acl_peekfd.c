#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <sys/ioctl.h>
#ifdef ACL_FIONREAD_IN_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef ACL_FIONREAD_IN_TERMIOS_H
#include <termios.h>
#endif
#include <unistd.h>
#include <sys/socket.h>

#endif

/* Utility library. */

#include "stdlib/acl_iostuff.h"

/* acl_peekfd - return amount of data ready to read */

int acl_peekfd(ACL_SOCKET fd)
{
	int     count;

	/*
	 * Anticipate a series of system-dependent code fragments.
	 */
#ifdef ACL_UNIX
	return (ioctl(fd, FIONREAD, (char *) &count) < 0 ? -1 : count);
#elif defined(ACL_WINDOWS)
	return (ioctlsocket(fd, FIONREAD, (unsigned long *) &count) < 0
		? -1 : count);
#endif
}
