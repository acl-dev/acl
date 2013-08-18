#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"
#endif

#ifdef	ACL_UNIX
#include <unistd.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "stdlib/unix/acl_trace.h"

void acl_dump_trace(const char *filepath)
{
	const char *myname = "acl_dump_trace";
	int   fd;
	void *buffer[1000];
	size_t n;

	n = backtrace(buffer, 1000);
	if (n == 0)
		return;

	fd = open(filepath, O_WRONLY | O_CREAT | O_APPEND, 0600);
	if (fd == -1) {
		acl_msg_error("%s(%d): open %s error(%s)",
			myname, __LINE__, filepath, acl_last_serror());
		return;
	}

	backtrace_symbols_fd(buffer, n, fd);
	close(fd);
}

#endif
