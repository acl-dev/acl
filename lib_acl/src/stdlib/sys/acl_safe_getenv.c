#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>

/* Utility library. */

#include "stdlib/acl_safe.h"

#endif

/* safe_getenv - read environment variable with guard */

char *acl_safe_getenv(const char *name)
{
#ifdef	ACL_UNIX
	return (acl_unsafe() == 0 ? getenv(name) : 0);
#elif defined(ACL_WINDOWS)
	return (getenv(name));
#endif
}

