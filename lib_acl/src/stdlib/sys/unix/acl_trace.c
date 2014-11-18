#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#endif

#ifdef	ACL_UNIX
#include <unistd.h>
# if !defined(ACL_SUNOS5) && !defined(ACL_FREEBSD)
#include <execinfo.h>
# endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stdlib/unix/acl_trace.h"

void acl_dump_trace(const char *filepath)
{
	const char *myname = "acl_dump_trace";
	int   fd;
#if	!defined(ACL_SUNOS5) && !defined(ACL_FREEBSD)
	void *buffer[1000];
#endif
	size_t n;

#if	!defined(ACL_SUNOS5) && !defined(ACL_FREEBSD)
	n = backtrace(buffer, 1000);
#else
	n = 0;
#endif
	if (n == 0)
		return;

	fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
	if (fd == -1) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return;
	}

#if	!defined(ACL_SUNOS5) && !defined(ACL_FREEBSD)
	backtrace_symbols_fd(buffer, n, fd);
#endif
	close(fd);
}

#endif
