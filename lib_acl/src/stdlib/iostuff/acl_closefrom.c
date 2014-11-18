#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_msg.h"

#endif

#ifdef ACL_UNIX
#include <errno.h>
#include <unistd.h>
#include "stdlib/acl_iostuff.h"

/* acl_closefrom() - closes all file descriptors from the given one up */
        
int acl_closefrom(int lowfd)
{   
	int   fd_limit = acl_open_limit(0);
	int   fd;

	/*
	 * lowfrom does not have an easy to determine upper limit. A process may
	 * have files open that were inherited from a parent process with a less
	 * restrictive resource limit.
	 */
	if (lowfd < 0) {
		acl_set_error(EBADF);
		return (-1);
	}
	if (fd_limit > 500)
		fd_limit = 500;
	for (fd = lowfd; fd < fd_limit; fd++)
		(void) close(fd);

	return (0);
}
#endif /* ACL_UNIX */
