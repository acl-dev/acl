#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

/* System library. */
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_make_dirs.h"

#endif

#include "dir_sys_patch.h"

/* acl_make_dirs - create directory hierarchy */

#ifdef ACL_UNIX

int acl_make_dirs(const char *path, int perms)
{
	char   *saved_path;
	unsigned char *cp;
	int     saved_ch;
	struct stat st;
	int     ret;
#if 0
	mode_t  saved_mode = 0;
#endif

	/*
	* Initialize. Make a copy of the path that we can safely clobber.
	*/
	cp = (unsigned char *) (saved_path = acl_mystrdup(path));

	/*
	 * I didn't like the 4.4BSD "mkdir -p" implementation, but coming up
	 * with my own took a day, spread out over several days.
	 */
#define SKIP_WHILE(cond, ptr) { while(*ptr && (cond)) ptr++; }

	SKIP_WHILE(*cp == '/', cp);

	for (;;) {
		SKIP_WHILE(*cp != '/', cp);
		if ((saved_ch = *cp) != 0)
			*cp = 0;
		if ((ret = stat(saved_path, &st)) >= 0) {
			if (!S_ISDIR(st.st_mode)) {
				acl_set_error(ENOTDIR);
				ret = -1;
				break;
			}
#if 0
			saved_mode = st.st_mode;
#endif
		} else {
			if (acl_last_error() != ENOENT)
				break;

			/*
			 * mkdir(foo) fails with EEXIST if foo is a symlink.
			 */
#if 0

			/*
			 * Create a new directory. Unfortunately, mkdir(2)
			 * has no equivalent of open(2)'s O_CREAT|O_EXCL
			 * safety net, so we must require that the parent
			 * directory is not world writable.
			 * Detecting a lost race condition after the fact is
			 * not sufficient, as an attacker could repeat the
			 * attack and add one directory level at a time.
			 */
			if (saved_mode & S_IWOTH) {
				acl_msg_warn("refusing to mkdir %s: "
						"parent directory is "
						"writable by everyone",
						saved_path);
				acl_set_error(EPERM);
				ret = -1;
				break;
			}
#endif

			if ((ret = mkdir(saved_path, perms)) < 0) {
				if (acl_last_error() != EEXIST)
					break;
				/* Race condition? */
				if ((ret = stat(saved_path, &st)) < 0)
					break;
				if (!S_ISDIR(st.st_mode)) {
					acl_set_error(ENOTDIR);
					ret = -1;
					break;
				}
			}
		}
		if (saved_ch != 0)
			*cp = saved_ch;
		SKIP_WHILE(*cp == '/', cp);
		if (*cp == 0)
			break;
	}

	/*
	 * Cleanup.
	 */
	acl_myfree(saved_path);
	return (ret);
}

#elif defined(ACL_WINDOWS)

int acl_make_dirs(const char *path, int perms)
{
	const char *myname = "acl_make_dirs";
	char   *saved_path;
	unsigned char *cp;
	int     saved_ch;
	struct stat st;
	int     ret;

	/*
	* Initialize. Make a copy of the path that we can safely clobber.
	*/
	cp = (unsigned char *) (saved_path = acl_mystrdup(path));

	if ((*cp >= 'a' && *cp <='z') || (*cp >= 'A' && *cp <= 'Z')) {
		if (*(cp + 1) == ':' && *(cp + 2) == '\\') {
			char  buf[4];

			buf[0] = *cp++;  /* 'a-z' | 'A-Z' */
			buf[1] = *cp++;  /* ':' */
			buf[2] = *cp++;  /* '\\' */
			buf[3] = 0;
			if (stat(buf, &st) < 0) {
				acl_msg_error("%s(%d): %s not exist(%s)",
					myname, __LINE__, strerror(errno));
				return (-1);
			}
		}
	}

	if (*cp == 0)
		return (0);

	/*
	* I didn't like the 4.4BSD "mkdir -p" implementation, but coming up
	* with my own took a day, spread out over several days.
	*/
#define SKIP_WHILE(cond, ptr) { while(*ptr && (cond)) ptr++; }

	SKIP_WHILE(*cp == '/' || *cp == '\\', cp);

	for (;;) {
		SKIP_WHILE(*cp != '/' && *cp != '\\', cp);

		if ((saved_ch = *cp) != 0)
			*cp = 0;
		if ((ret = stat(saved_path, &st)) >= 0) {
			if (!S_ISDIR(st.st_mode)) {
				errno = ENOTDIR;
				acl_set_error(ERROR_PATH_NOT_FOUND);
				ret = -1;
				break;
			}
		} else {
			if (errno != ENOENT)
				break;

			/*if ((ret = mkdir(saved_path)) < 0) {*/
			if (!CreateDirectory(saved_path, NULL)) {
				int  error = acl_last_error();

				if (error != ERROR_ALREADY_EXISTS)
					break;
				ret = 0;
				/* Race condition? */
				if ((ret = stat(saved_path, &st)) < 0)
					break;
				if (!S_ISDIR(st.st_mode)) {
					acl_set_error(ENOTDIR);
					ret = -1;
					break;
				}
			} else
				ret = 0;
		}
		if (saved_ch != 0)
			*cp = saved_ch;
		SKIP_WHILE(*cp == '/' || *cp == '\\', cp);
		if (*cp == 0)
			break;
	}

	/*
	* Cleanup.
	*/
	acl_myfree(saved_path);
	return (ret);
}

#else
# error "unknown OS"
#endif

