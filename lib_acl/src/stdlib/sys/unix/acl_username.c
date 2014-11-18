#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include "stdlib/unix/acl_username.h"

const char *acl_username(void)
{
	uid_t   uid;
	struct passwd *pwd;

	uid = getuid();
	if ((pwd = getpwuid(uid)) == 0)
		return (0);
	return (pwd->pw_name);
}
#endif /* ACL_UNIX */
