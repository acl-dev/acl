/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#include <grp.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_chroot_uid.h"

/* chroot_uid - restrict the damage that this program can do */

int  acl_chroot_uid(const char *root_dir, const char *user_name)
{
	struct passwd *pwd;
	uid_t  uid = 0;
	gid_t  gid;
	int    err = 0;

	/*
	 * Look up the uid/gid before entering the jail, and save them so they
	 * can't be clobbered. Set up the primary and secondary groups.
	 */
	if (user_name != 0) {
		if ((pwd = getpwnam(user_name)) == 0) {
			acl_msg_error("unknown user: %s", user_name);
			return -1;
		}

		uid = pwd->pw_uid;
		gid = pwd->pw_gid;
		if (setgid(gid) < 0) {
			acl_msg_error("setgid(%ld): %s", (long) gid,
				acl_last_serror());
			err++;
		}
		if (initgroups(user_name, gid) < 0) {
			acl_msg_error("initgroups: %s", acl_last_serror());
			err++;
		}
	}

	/*
	 * Enter the jail.
	 */
	if (root_dir) {
		if (chroot(root_dir)) {
			acl_msg_error("chroot(%s): %s",
				root_dir, acl_last_serror());
			err++;
		} else if (chdir("/")) {
			acl_msg_error("chdir(/): %s", acl_last_serror());
			err++;
		}
	}

	/*
	 * Drop the user privileges.
	 */
	if (user_name != 0) {
		if (setuid(uid) < 0) {
			acl_msg_error("setuid(%ld): %s",
				(long) uid, acl_last_serror());
			err++;
		}
	}

	/*
	 * Give the desperate developer a clue of what is happening.
	 */
	if (acl_msg_verbose > 1)
		acl_msg_info("chroot %s user %s",
			root_dir ? root_dir : "(none)",
			user_name ? user_name : "(none)");

	return err ? -1 : 0;
}
#endif /* ACL_UNIX */
