#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System library. */
#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/unix/acl_safe_open.h"

/* safe_open_exist - open existing file */

static ACL_VSTREAM *acl_safe_open_exist(const char *path, int flags,
	struct stat * fstat_st, ACL_VSTRING *why)
{
	struct stat local_statbuf;
	struct stat lstat_st;
	int     saved_error;
	ACL_VSTREAM *fp;
	char  tbuf[256];

	/*
	 * Open an existing file.
	 */
	fp = acl_vstream_fopen(path, flags & ~(O_CREAT | O_EXCL), 0, 4096);
	if (fp == 0) {
		saved_error = acl_last_error();
		if (why) {
			acl_vstring_sprintf(why, "cannot open file %s: %s",
				path, acl_last_strerror(tbuf, sizeof(tbuf)));
		}
		acl_set_error(saved_error);
		return (0);
	}

	/*
	 * Examine the modes from the open file: it must have exactly one hard
	 * link (so that someone can't lure us into clobbering a sensitive file
	 * by making a hard link to it), and it must be a non-symlink file.
	 */
	if (fstat_st == 0)
		fstat_st = &local_statbuf;
	if (fstat(ACL_VSTREAM_FILE(fp), fstat_st) < 0) {
		acl_msg_fatal("%s: bad open file status: %s", path,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	} else if (S_ISDIR(fstat_st->st_mode)) {
		if (why)
			acl_vstring_sprintf(why, "file is a directory");
		acl_set_error(EISDIR);
	} else if (fstat_st->st_nlink != 1) {
		if (why)
			acl_vstring_sprintf(why, "file has %d hard links",
					(int) fstat_st->st_nlink);
		acl_set_error(EPERM);
	}

	/*
	 * Look up the file again, this time using lstat(). Compare the fstat()
	 * (open file) modes with the lstat() modes. If there is any difference,
	 * either we followed a symlink while opening an existing file, someone
	 * quickly changed the number of hard links, or someone replaced the file
	 * after the open() call. The link and mode tests aren't really necessary
	 * in daemon processes. Set-uid programs, on the other hand, can be
	 * slowed down by arbitrary amounts, and there it would make sense to
	 * compare even more file attributes, such as the inode generation number
	 * on systems that have one.
	 * 
	 * Grr. Solaris /dev/whatever is a symlink. We'll have to make an exception
	 * for symlinks owned by root. NEVER, NEVER, make exceptions for symlinks
	 * owned by a non-root user. This would open a security hole when
	 * delivering mail to a world-writable mailbox directory.
	 */
	else if (lstat(path, &lstat_st) < 0) {
		if (why)
			acl_vstring_sprintf(why, "file status changed unexpectedly: %s",
				acl_last_strerror(tbuf, sizeof(tbuf)));
		acl_set_error(EPERM);
	} else if (S_ISLNK(lstat_st.st_mode)) {
		if (lstat_st.st_uid == 0)
			return (fp);
		if (why)
			acl_vstring_sprintf(why, "file is a symbolic link");
		acl_set_error(EPERM);
	} else if (fstat_st->st_dev != lstat_st.st_dev
		   || fstat_st->st_ino != lstat_st.st_ino
#ifdef HAS_ST_GEN
		   || fstat_st->st_gen != lstat_st.st_gen
#endif
		   || fstat_st->st_nlink != lstat_st.st_nlink
		   || fstat_st->st_mode != lstat_st.st_mode) {
		if (why)
			acl_vstring_sprintf(why, "file status changed unexpectedly");
		acl_set_error(EPERM);
	}

	/*
	 * We are almost there...
	 */
	else {
		return (fp);
	}

	/*
	 * End up here in case of fstat()/lstat() problems or inconsistencies.
	 */
	acl_vstream_fclose(fp);
	return (0);
}

/* acl_safe_open_create - create new file */

static ACL_VSTREAM *acl_safe_open_create(const char *path, int flags, int mode,
	struct stat * st, uid_t user, uid_t group, ACL_VSTRING *why)
{
	ACL_VSTREAM *fp;
	char tbuf[256];

	/*
	 * Create a non-existing file. This relies on O_CREAT | O_EXCL to not
	 * follow symbolic links.
	 */
	fp = acl_vstream_fopen(path, flags | (O_CREAT | O_EXCL), mode, 4096);
	if (fp == 0) {
		if (why)
			acl_vstring_sprintf(why, "cannot create file exclusively: %s",
				acl_last_strerror(tbuf, sizeof(tbuf)));
		return (0);
	}

	/*
	 * Optionally change ownership after creating a new file. If there
	 * is a problem we should not attempt to delete the file. Something
	 * else may have opened the file in the mean time.
	 */
#define CHANGE_OWNER(user, group) (user != (uid_t) -1 || group != (gid_t) -1)

	if (CHANGE_OWNER(user, group) && fchown(ACL_VSTREAM_FILE(fp), user, group) < 0) {
		acl_msg_warn("%s: cannot change file ownership: %s",
			path, acl_last_strerror(tbuf, sizeof(tbuf)));
	}

	/*
	 * Optionally look up the file attributes.
	 */
	if (st != 0 && fstat(ACL_VSTREAM_FILE(fp), st) < 0)
		acl_msg_fatal("%s: bad open file status: %s",
			path, acl_last_strerror(tbuf, sizeof(tbuf)));

	/*
	 * We are almost there...
	 */
	else {
		return (fp);
	}

	/*
	 * End up here in case of trouble.
	 */
	acl_vstream_fclose(fp);
	return (0);
}

/* acl_safe_open - safely open or create file */

ACL_VSTREAM *acl_safe_open(const char *path, int flags, int mode,
	struct stat * st, uid_t user, gid_t group, ACL_VSTRING *why)
{
	ACL_VSTREAM *fp;

	switch (flags & (O_CREAT | O_EXCL)) {

	/*
	 * Open an existing file, carefully.
	 */
	case 0:
		return (acl_safe_open_exist(path, flags, st, why));

		/*
		 * Create a new file, carefully.
		 */
	case O_CREAT | O_EXCL:
		return (acl_safe_open_create(path, flags, mode,
						st, user, group, why));

		/*
		 * Open an existing file or create a new one, carefully.
		 * When opening an existing file, we are prepared to deal
		 * with "no file" errors only. When creating a file, we
		 * are prepared for "file exists" errors only. Any other
		 * error means we better give up trying.
		 */
	case O_CREAT:
		fp = acl_safe_open_exist(path, flags, st, why);
		if (fp == 0 && acl_last_error() == ENOENT) {
			fp = acl_safe_open_create(path, flags, mode, st,
						user, group, why);
			if (fp == 0 && acl_last_error() == EEXIST)
				fp = acl_safe_open_exist(path, flags, st, why);
		}
		return (fp);

		/*
		 * Interface violation. Sorry, but we must be strict.
		 */
	default:
		acl_msg_panic("acl_safe_open: O_EXCL flag without O_CREAT flag");
	}

	/* no reache here */

	return (NULL);
}

#endif /* ACL_UNIX */

