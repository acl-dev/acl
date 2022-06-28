#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_WINDOWS
#pragma once
#endif

#ifdef ACL_UNIX
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"

#endif

/* doze - sleep a while */

void acl_doze(unsigned delay)
{
#ifdef	ACL_UNIX
	struct timeval tv;

	tv.tv_sec = delay / 1000;
	tv.tv_usec = (suseconds_t) (delay - tv.tv_sec * 1000) * 1000;
	while (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &tv) < 0) {
		if (acl_last_error() != ACL_EINTR) {
			char tbuf[256];
			acl_msg_fatal("doze: select: %s",
				acl_last_strerror(tbuf, sizeof(tbuf)));
		}
	}
#elif	defined(ACL_WINDOWS)
	Sleep(delay);
#else
#error "unknown OS"
#endif
}
