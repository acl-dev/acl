/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <unistd.h>
#include <grp.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_set_eugid.h"

/* set_eugid - set effective user and group attributes */

int acl_set_eugid(uid_t euid, gid_t egid)
{
	int   saved_error = acl_last_error();
	char  tbuf[256];

	if (geteuid() != 0 && seteuid(0)) {
		acl_msg_error("set_eugid: seteuid(0): %s",
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	if (setegid(egid) < 0) {
		acl_msg_error("set_eugid: setegid(%ld): %s", (long) egid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
#ifndef MINGW
	if (setgroups(1, &egid) < 0) {
		acl_msg_error("set_eugid: setgroups(%ld): %s", (long) egid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
#endif
	if (euid != 0 && seteuid(euid) < 0) {
		acl_msg_error("set_eugid: seteuid(%ld): %s", (long) euid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	if (acl_msg_verbose)
		acl_msg_info("set_eugid: euid %ld egid %ld",
			(long) euid, (long) egid);

	acl_set_error(saved_error);
	return 0;
}
#endif /* ACL_UNIX */
