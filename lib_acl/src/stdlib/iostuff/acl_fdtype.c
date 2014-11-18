#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "stdlib/acl_iostuff.h"

int  acl_issock(int fd)
{
	struct stat     buf;

	if (fstat(fd, &buf) < 0)
		return(0);

	if ((buf.st_mode & S_IFMT) == S_IFSOCK)
		return(1);
	else
		return(0);
}
#endif /* ACL_UNIX*/

