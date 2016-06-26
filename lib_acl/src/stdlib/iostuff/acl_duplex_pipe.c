#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_iostuff.h"

#endif

#ifdef ACL_UNIX
#include <sys/socket.h>
#include <unistd.h>
#include "stdlib/acl_sys_patch.h"
#endif

#ifdef	ACL_WINDOWS
#define	HAS_DUPLEX_PIPE
#endif

/* duplex_pipe - give me a duplex pipe or bust */

int acl_duplex_pipe(ACL_FILE_HANDLE fds[2])
{
#if defined(HAS_DUPLEX_PIPE)
	return (acl_pipe(fds));
#else
	return (acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds));
#endif
}



