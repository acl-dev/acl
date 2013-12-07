#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#endif

#ifdef	ACL_UNIX
#include <unistd.h>
# ifndef ACL_SUNOS5
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
#ifndef	ACL_SUNOS5
	void *buffer[1000];
#endif
	size_t n;

#ifndef	ACL_SUNOS5
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

#ifndef	ACL_SUNOS5
	backtrace_symbols_fd(buffer, n, fd);
#endif
	close(fd);
}

#endif
