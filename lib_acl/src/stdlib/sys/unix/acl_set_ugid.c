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

void acl_set_ugid(uid_t uid, gid_t gid)
{
	int   saved_error = acl_last_error();
	char  tbuf[256];

	if (geteuid() != 0 && seteuid(0) < 0)
		acl_msg_fatal("seteuid(0): %s",
			acl_last_strerror(tbuf, sizeof(tbuf)));
	if (setgid(gid) < 0)
		acl_msg_fatal("setgid(%ld): %s", (long) gid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	if (setgroups(1, &gid) < 0)
		acl_msg_fatal("setgroups(1, &%ld): %s", (long) gid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	if (setuid(uid) < 0)
		acl_msg_fatal("setuid(%ld): %s", (long) uid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	if (acl_msg_verbose > 1)
		acl_msg_info("setugid: uid %ld gid %ld", (long) uid, (long) gid);
	acl_set_error(saved_error);
}

int acl_change_uid(char *user_name)
{
	const char *myname = "change_uid";
	struct passwd *pwd;
	uid_t  uid;
	gid_t  gid;
	char  tbuf[256];

	if ((pwd = getpwnam(user_name)) == NULL)
		acl_msg_fatal("%s: no such user=%s", myname, user_name);
	uid = pwd->pw_uid;
	gid = pwd->pw_gid;
	if (setgid(gid) < 0)
		acl_msg_fatal("%s: setgid error(%s, %d): %s",
			myname, user_name, (int) uid,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	if (setuid(uid) < 0)
		acl_msg_fatal("%s: setuid error(%s, %d): %s",
			myname, user_name, (int) uid,
			acl_last_strerror(tbuf, sizeof(tbuf)));

	return (0);
}
#endif /* ACL_UNIX*/
