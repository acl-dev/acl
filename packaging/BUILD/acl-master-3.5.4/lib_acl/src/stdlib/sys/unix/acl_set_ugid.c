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
#include <pwd.h>
#include <sys/types.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_set_ugid.h"

int acl_set_ugid(uid_t uid, gid_t gid)
{
	int   saved_error = acl_last_error();
	char  tbuf[256];

	if (geteuid() != 0 && seteuid(0) < 0) {
		acl_msg_error("seteuid(0): %s",
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	if (setgid(gid) < 0) {
		acl_msg_error("setgid(%ld): %s", (long) gid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
#ifndef MINGW
	if (setgroups(1, &gid) < 0) {
		acl_msg_error("setgroups(1, &%ld): %s", (long) gid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
#endif
	if (setuid(uid) < 0) {
		acl_msg_error("setuid(%ld): %s", (long) uid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	if (acl_msg_verbose > 1)
		acl_msg_info("setugid: uid %ld gid %ld",
			(long) uid, (long) gid);
	acl_set_error(saved_error);
	return 0;
}

int acl_change_uid(const char *user)
{
	const char *myname = "change_uid";
	struct passwd *pwd;
	uid_t  uid;
	gid_t  gid;
	char  tbuf[256];

	if ((pwd = getpwnam(user)) == NULL) {
		acl_msg_error("%s: no such user: %s", myname, user);
		return -1;
	}
	uid = pwd->pw_uid;
	gid = pwd->pw_gid;
	if (setgid(gid) < 0) {
		acl_msg_error("%s: setgid error(%s, %d): %s", myname, user,
			(int) uid, acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	if (setuid(uid) < 0) {
		acl_msg_error("%s: setuid error(%s, %d): %s", myname, user,
			(int) uid, acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}

	return 0;
}
#endif /* ACL_UNIX*/
