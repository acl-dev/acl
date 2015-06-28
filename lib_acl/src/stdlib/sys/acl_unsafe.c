#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_safe.h"

/* System library. */
#ifdef	ACL_UNIX
#include <unistd.h>
#endif

#endif

/* acl_unsafe - can we trust user-provided environment, working directory, etc. */

#ifdef	ACL_UNIX
int acl_unsafe(void)
{
	return (geteuid() != getuid()
#ifdef HAS_ISSETUGID
		|| issetugid()
#endif
		|| getgid() != getegid());
}
#elif defined(ACL_WINDOWS)
int acl_unsafe(void)
{
	return (0);
}
#endif
