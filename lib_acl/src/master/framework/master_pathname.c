/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

/* Utility library. */

#include "stdlib/acl_stringops.h"

/* Global library. */

#include "master_pathname.h"

/* acl_master_pathname - map service class and service name to pathname */

char   *acl_master_pathname(const char *queue_path, const char *service_class,
	const char *service_name)
{
	return (acl_concatenate(queue_path, "/", service_class, "/",
			service_name, (char *) 0));
}
#endif /* ACL_UNIX */
