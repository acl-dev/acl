/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef	ACL_UNIX

#include <unistd.h>
#include <fcntl.h>

/* Utility library. */

#include "stdlib/acl_vstream.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_safe_open.h"
#include "stdlib/unix/acl_open_lock.h"

/* open_lock - open file and lock it for exclusive access */

ACL_VSTREAM *acl_open_lock(const char *path, int flags,
	int mode, ACL_VSTRING *why)
{
	ACL_VSTREAM *fp;

	/*
	 * Carefully create or open the file, and lock it down. Some systems
	 * don't have the O_LOCK open() flag, or the flag does not do what we
	 * want, so we roll our own lock.
	 */
	fp = acl_safe_open(path, flags, mode, (struct stat *) 0, -1, -1, why);
	if (fp == 0)
		return 0;

	if (acl_myflock(ACL_VSTREAM_FILE(fp), ACL_INTERNAL_LOCK,
		ACL_FLOCK_OP_EXCLUSIVE | ACL_FLOCK_OP_NOWAIT) < 0)
	{
		acl_vstring_sprintf(why, "unable to set exclusive lock: %s",
			acl_last_serror());
		acl_vstream_close(fp);
		return 0;
	}
	return fp;
}
#endif /* ACL_UNIX */
