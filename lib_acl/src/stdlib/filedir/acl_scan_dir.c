#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
/* System library. */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef ACL_UNIX
#include <dirent.h>
#include <unistd.h>
#elif	defined(ACL_BCB_COMPILER)
#include <dirent.h>
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_scan_dir.h"

#endif

#include "stdlib/acl_dir.h"

#ifdef ACL_WINDOWS
# define SANE_RMDIR _rmdir
# define SANE_UNLINK _unlink
#else
# define SANE_RMDIR rmdir
# define SANE_UNLINK unlink
#endif

#include "dir_sys_patch.h"

#ifndef	MAX_PATH
#define	MAX_PATH	1024
#endif

 /*
  * The interface is based on an opaque structure, so we don't have to expose
  * the user to the guts. Subdirectory info sits in front of parent directory
  * info: a simple last-in, first-out list.
  */
typedef struct ACL_SCAN_INFO ACL_SCAN_INFO;

struct ACL_SCAN_INFO {
	char   *path;               /* directory name */
	DIR    *dir_name;           /* directory structure */
	unsigned nfiles;
	unsigned ndirs;
	acl_int64 nsize;            /* total size of all files of current */
	struct acl_stat sbuf;       /* the stat of the dir or file of scanning */
	struct acl_stat attr;       /* the current dir's stat */
	ACL_SCAN_INFO *parent;      /* linkage */
};

struct ACL_SCAN_DIR {
	ACL_SCAN_INFO *current;     /* current scan */
	unsigned flags;             /* define as: ACL_SCAN_FLAGS_XXX */
	unsigned nfiles;            /* total files' count */
	unsigned ndirs;             /* total dirs' count */
	acl_int64 nsize;            /* total size of all files */
	ACL_SCAN_DIR_FN   scan_fn;
	ACL_SCAN_RMDIR_FN rmdir_fn;
	void *scan_ctx;
	char  file_name[256];       /* if is file set it */
};

#define SCAN_PATH(scan)		(scan->current->path)
#define SCAN_RECURSIVE(scan)	(scan->flags & ACL_SCAN_FLAG_RECURSIVE)
#define SCAN_RMDIR(scan)	(scan->flags & ACL_SCAN_FLAG_RMDIR)

/* acl_scan_dir_open - start directory scan */

ACL_SCAN_DIR *acl_scan_dir_open(const char *path, int recursive)
{
	return acl_scan_dir_open2(path, recursive ? ACL_SCAN_FLAG_RECURSIVE : 0);
}

ACL_SCAN_DIR *acl_scan_dir_open2(const char *path, unsigned flags)
{
	const char *myname = "acl_scan_dir_open2";
	ACL_SCAN_DIR *scan;

	scan = (ACL_SCAN_DIR *) acl_mycalloc(1, sizeof(*scan));
	scan->flags = flags;

	if (acl_scan_dir_push(scan, path) < 0) {
		return NULL;
	}

	if (acl_stat(path, &scan->current->attr) < 0) {
		acl_msg_error("%s(%d), %s: stat %s error %s", __FILE__,
			__LINE__, myname, path, acl_last_serror());
		acl_scan_dir_close(scan);
		return NULL;
	}

	return scan;
}

/* acl_scan_dir_close - terminate directory scan */

void acl_scan_dir_close(ACL_SCAN_DIR *scan)
{
	while (scan->current) {
		acl_scan_dir_pop(scan);
	}
	acl_myfree(scan);
}

void acl_scan_dir_reset(ACL_SCAN_DIR *scan)
{
	scan->nfiles = 0;
	scan->ndirs  = 0;
	scan->nsize  = 0;
}

/* acl_scan_dirctl - ctl interface for set option */

void acl_scan_dir_ctl(ACL_SCAN_DIR *scan, int name, ...)
{
	va_list ap;

	va_start(ap, name);

	for (; name != ACL_SCAN_CTL_END; name = va_arg(ap, int)) {
		switch(name) {
		case ACL_SCAN_CTL_FN:
			scan->scan_fn = va_arg(ap, ACL_SCAN_DIR_FN);
			break;
		case ACL_SCAN_CTL_RMDIR_FN:
			scan->rmdir_fn = va_arg(ap, ACL_SCAN_RMDIR_FN);
			break;
		case ACL_SCAN_CTL_CTX:
			scan->scan_ctx = va_arg(ap, void*);
			break;
		default:
			break;
		}
	}

	va_end(ap);
}

/* acl_scan_dir_path - return the path of the directory being read.  */

const char *acl_scan_dir_path(ACL_SCAN_DIR *scan)
{
	if (scan->current == NULL) {
		return NULL;
	}
	return SCAN_PATH(scan);
}

const char *acl_scan_dir_file(ACL_SCAN_DIR *scan)
{
	if (scan->current == NULL) {
		return NULL;
	}

	return scan->file_name;
}

unsigned acl_scan_dir_ndirs(ACL_SCAN_DIR *scan)
{
	return scan->ndirs;
}

unsigned acl_scan_dir_nfiles(ACL_SCAN_DIR *scan)
{
	return scan->nfiles;
}

acl_int64 acl_scan_dir_nsize(ACL_SCAN_DIR *scan)
{
	return scan->nsize;
}

int acl_scan_stat(ACL_SCAN_DIR *scan, struct acl_stat *sbuf)
{
	if (scan->current == NULL || sbuf == NULL) {
		return -1;
	}

	memcpy(sbuf, &scan->current->sbuf, sizeof(struct acl_stat));
	return 0;
}

int acl_scan_dir_stat(ACL_SCAN_DIR *scan, struct acl_stat *sbuf)
{
	if (scan->current == NULL || sbuf == NULL) {
		return -1;
	}

	memcpy(sbuf, &scan->current->attr, sizeof(struct acl_stat));
	return 0;
}

int acl_scan_dir_end(ACL_SCAN_DIR *scan)
{
	if (scan->current == NULL) {
		return 1;
	} else {
		return 0;
	}
}
/* acl_scan_dir_push - enter directory */

int acl_scan_dir_push(ACL_SCAN_DIR *scan, const char *path)
{
	const char *myname = "acl_scan_dir_push";
	ACL_SCAN_INFO *info;

	if (path == NULL || *path == 0) {
		acl_msg_fatal("%s(%d), %s: path null",
			__FILE__, __LINE__, myname);
	}

	info = (ACL_SCAN_INFO *) acl_mycalloc(1, sizeof(*info));
	if (scan->current) {
		const char *dpath = SCAN_PATH(scan);

		if (*dpath == '/' && *(dpath + 1) == 0) {
			info->path = acl_concatenate(dpath, path, NULL);
		} else {
			info->path = acl_concatenate(dpath, PATH_SEP_S, path, NULL);
		}
	} else {
		size_t len = strlen(path);
		const char *ptr = path + len - 1;

#ifdef	ACL_WINDOWS
		while (ptr > path && (*ptr == '/' || *ptr == '\\')) {
#else
		while (ptr > path && *ptr == '/') {
#endif
			ptr--;
		}
		len = ptr - path + 1;
		info->path = (char*) acl_mymalloc(len + 1);
		memcpy(info->path, path, len);
		info->path[len] = 0;
	}

	if ((info->dir_name = opendir(info->path)) == 0) {
		acl_msg_error("%s(%d), %s: open directory(%s) error(%s)",
			__FILE__, __LINE__, myname,
			info->path, acl_last_serror());
		acl_myfree(info->path);
		acl_myfree(info);
		return -1;
	}

	info->parent = scan->current;
	scan->current = info;
	return 0;
}

/* acl_scan_dir_pop - leave directory */

ACL_SCAN_DIR *acl_scan_dir_pop(ACL_SCAN_DIR *scan)
{
	const char *myname = "acl_scan_dir_pop";
	ACL_SCAN_INFO *info = scan->current;
	ACL_SCAN_INFO *parent;

	if (info == NULL) {
		return NULL;
	}

	parent = info->parent;
	if (closedir(info->dir_name)) {
		acl_msg_error("%s(%d), %s: close directory(%s) error(%s)",
			__FILE__, __LINE__, myname,
			info->path, acl_last_serror());
	}

	if (SCAN_RMDIR(scan) && parent && !info->ndirs && !info->nfiles
		&& scan->rmdir_fn
		&& scan->rmdir_fn(scan, info->path, scan->scan_ctx) == 0) {

		if (parent->ndirs > 0) {
			parent->ndirs--;
		} else {
			acl_msg_error("%s(%d), %s: parent=%s ndirs=0",
				__FILE__, __LINE__, myname, parent->path);
		}

		if (scan->ndirs > 0) {
			scan->ndirs--;
		} else {
			acl_msg_error("%s(%d), %s: total ndirs=0",
				__FILE__, __LINE__, myname);
		}
	}

	acl_myfree(info->path);
	acl_myfree(info);
	scan->current = parent;

	return parent ? scan : NULL;
}

/* acl_scan_dir_next - find next entry */

const char *acl_scan_dir_next(ACL_SCAN_DIR *scan)
{
	const char *myname = "acl_scan_dir_next";
	ACL_SCAN_INFO *info = scan->current;
	struct dirent *dp;

#define STREQ(x,y)	(strcmp((x),(y)) == 0)

	if (info == NULL) {
		return NULL;
	}

	while (1) {
		acl_set_error(0);
		dp = readdir(info->dir_name);
		if (dp == NULL) {
			int err = acl_last_error();
#ifdef ACL_WINDOWS
			if (err != 0 && err != ERROR_NO_MORE_FILES) {
#else
			if (err != 0) {
#endif
				acl_msg_error("%s(%d), %s: readdir error=%s, "
					"name=%s", __FILE__, __LINE__, myname,
					acl_last_serror(), info->path);
			}
			break;
		}

		if (STREQ(dp->d_name, ".") || STREQ(dp->d_name, "..")) {
			continue;
		}
		return dp->d_name;
	}

	return NULL;
}

/* acl_scan_dir_next_file - find next valid file */

const char *acl_scan_dir_next_file(ACL_SCAN_DIR *scan)
{
	const char *myname = "acl_scan_dir_next_file";
	const char *name;
	char  pathbuf[MAX_PATH];
	struct acl_stat sbuf;

	for (;;) {
		if ((name = acl_scan_dir_next(scan)) == NULL) {
			if (acl_scan_dir_pop(scan) == 0) {
				return NULL;
			}
			continue;
		}

		snprintf(pathbuf, sizeof(pathbuf), "%s%c%s",
			SCAN_PATH(scan), PATH_SEP_C, name);

		if (acl_stat(pathbuf, &sbuf) < 0) {
			acl_msg_error("%s(%d), %s: stat file(%s) error(%s)",
				__FILE__, __LINE__, myname, pathbuf,
				acl_last_serror());
			continue;
		}

		memcpy(&scan->current->sbuf, &sbuf, sizeof(sbuf));

		scan->nsize += sbuf.st_size;
		scan->current->nsize += sbuf.st_size;

		if (!S_ISDIR(sbuf.st_mode)) {
			scan->nfiles++;
			scan->current->nfiles++;
			return name;
		}

		scan->ndirs++;
		scan->current->ndirs++;

		if (!SCAN_RECURSIVE(scan)) {
			/* none */
		} else if (acl_scan_dir_push(scan, name) == 0) {
			memcpy(&scan->current->attr, &sbuf, sizeof(sbuf));
		} else {
			acl_msg_error("%s(%d), %s: push dir(%s) error %s",
				__FILE__, __LINE__, myname,
				pathbuf, acl_last_serror());
		}
	}
}

/* acl_scan_dir_next_dir - find next valid dir */

const char *acl_scan_dir_next_dir(ACL_SCAN_DIR *scan)
{
	const char *myname = "acl_scan_dir_next_dir";
	const char *name;
	char  pathbuf[MAX_PATH];
	struct acl_stat sbuf;

	for (;;) {
		if ((name = acl_scan_dir_next(scan)) == NULL) {
			if (acl_scan_dir_pop(scan) == 0) {
				return NULL;
			}
			continue;
		}
		snprintf(pathbuf, sizeof(pathbuf), "%s%c%s",
			SCAN_PATH(scan), PATH_SEP_C, name);
		if (acl_stat(pathbuf, &sbuf) < 0) {
			acl_msg_error("%s(%d), %s: stat file(%s) error(%s)",
				__FILE__, __LINE__, myname, pathbuf,
				acl_last_serror());
			continue;
		}

		memcpy(&scan->current->sbuf, &sbuf, sizeof(sbuf));

		scan->nsize += sbuf.st_size;
		scan->current->nsize += sbuf.st_size;

		if (!S_ISDIR(sbuf.st_mode)) {
			scan->nfiles++;
			scan->current->nfiles++;
			continue;
		}

		scan->ndirs++;
		scan->current->ndirs++;

		if (!SCAN_RECURSIVE(scan)) {
			return name;
		}

		if (acl_scan_dir_push(scan, name) == 0) {
			memcpy(&scan->current->attr, &sbuf, sizeof(sbuf));
			return name;
		}

		acl_msg_error("%s(%d), %s: push dir(%s) error %s", __FILE__,
			__LINE__, myname, pathbuf, acl_last_serror());
	}
}

const char *acl_scan_dir_next_name(ACL_SCAN_DIR *scan, int *is_file)
{
	const char *myname = "acl_scan_dir_next_name";
	const char *name;
	char  pathbuf[MAX_PATH];
	struct acl_stat sbuf;

	for (;;) {
		if ((name = acl_scan_dir_next(scan)) == NULL) {
			if (acl_scan_dir_pop(scan) == 0) {
				return NULL;
			}
			continue;
		}

		snprintf(pathbuf, sizeof(pathbuf), "%s%c%s",
			SCAN_PATH(scan), PATH_SEP_C, name);

		if (acl_stat(pathbuf, &sbuf) < 0) {
			acl_msg_error("%s(%d), %s: stat file(%s) error(%s)",
				__FILE__, __LINE__, myname, pathbuf,
				acl_last_serror());
			continue;
		}

		memcpy(&scan->current->sbuf, &sbuf, sizeof(sbuf));
		scan->nsize += sbuf.st_size;
		scan->current->nsize += sbuf.st_size;

		if (!S_ISDIR(sbuf.st_mode)) {
			if (is_file) {
				*is_file = 1;
			}
			scan->nfiles++;
			scan->current->nfiles++;
			return name;
		}

		scan->ndirs++;
		scan->current->ndirs++;

		if (!SCAN_RECURSIVE(scan)) {
			if (is_file) {
				*is_file = 0;
			}
			return name;
		}

		if (acl_scan_dir_push(scan, name) == 0) {
			if (is_file) {
				*is_file = 0;
			}
			memcpy(&scan->current->attr, &sbuf, sizeof(sbuf));
			return name;
		}

		acl_msg_error("%s(%d), %s: push dir(%s) error %s", __FILE__,
			__LINE__, myname, pathbuf, acl_last_serror());
	}
}

acl_int64 acl_scan_dir_size2(ACL_SCAN_DIR *scan, int *nfile, int *ndir)
{
	const char *myname = "acl_scan_dir_size2";
	const char *name;
	char  pathbuf[MAX_PATH];
	struct acl_stat sbuf;

	while (1) {
		if ((name = acl_scan_dir_next(scan)) == NULL) {
			if (acl_scan_dir_pop(scan) == NULL) {
				break;
			}
			continue;
		}
		snprintf(pathbuf, sizeof(pathbuf), "%s%c%s",
			SCAN_PATH(scan), PATH_SEP_C, name);
		if (acl_stat(pathbuf, &sbuf) < 0) {
			acl_msg_error("%s(%d), %s: stat file(%s) error(%s)",
				__FILE__, __LINE__, myname, pathbuf,
				acl_last_serror());
			continue;
		}

		memcpy(&scan->current->sbuf, &sbuf, sizeof(sbuf));

		if (S_ISDIR(sbuf.st_mode)) {
			scan->ndirs++;
			scan->file_name[0] = 0;

			if (SCAN_RECURSIVE(scan)
				&& acl_scan_dir_push(scan, name) < 0) {

				continue;
			}
		} else {
			ACL_SAFE_STRNCPY(scan->file_name, name,
				sizeof(scan->file_name));
			scan->nfiles++;
			scan->nsize += sbuf.st_size;
		}

		if (scan->scan_fn && scan->scan_fn(scan, scan->scan_ctx) < 0) {
			break;
		}
	}

	if (nfile) {
		*nfile = scan->nfiles;
	}
	if (ndir) {
		*ndir = scan->ndirs;
	}

	return scan->nsize;
}

acl_int64 acl_scan_dir_size(const char *pathname, int recursive,
	int *nfile, int *ndir)
{
	const char *myname = "acl_scan_dir_size";
	ACL_SCAN_DIR *scan;
	acl_int64 size;

	if (pathname == NULL || *pathname == 0) {
		acl_msg_error("%s(%d), %s: pathname null",
			__FILE__, __LINE__, myname);
		return -1;
	}

	scan = acl_scan_dir_open(pathname, recursive);
	if (scan == NULL) {
		acl_msg_error("%s(%d), %s: dir_open error: %s, path: %s",
			__FILE__, __LINE__, myname,
			acl_last_serror(), pathname);
		return -1;
	}

	size = acl_scan_dir_size2(scan, nfile, ndir);
	acl_scan_dir_close(scan);

	return size;
}

/* acl_scan_dir_rmall - remove all directoies and file in the dir */

acl_int64 acl_scan_dir_rm2(ACL_SCAN_DIR *scan, int *ndir, int *nfile)
{
	const char *myname = "acl_scan_dir_rm2";
	const char *name;
	char  path[MAX_PATH];
	struct acl_stat sbuf;

	for (;;) {
		if ((name = acl_scan_dir_next(scan)) == NULL) {
			if (scan->current != NULL) {
				snprintf(path, sizeof(path), "%s",
					SCAN_PATH(scan));
			} else {
				path[0] = 0;
			}

			/* 必须退出该空目录后才可以删除该目录 */

			if (acl_scan_dir_pop(scan) == 0) {
				/* 删除最顶层空目录 */
				if (path[0] != 0 && SANE_RMDIR(path) == 0) {
					scan->ndirs++;
				}
				break;
			}

			/* 删除空目录 */
			if (path[0] != 0 && SANE_RMDIR(path) == 0) {
				scan->ndirs++;
			}
			continue;
		}

		snprintf(path, sizeof(path), "%s%c%s",
			SCAN_PATH(scan), PATH_SEP_C, name);

		if (acl_stat(path, &sbuf) < 0) {
			acl_msg_error("%s(%d), %s: stat file(%s) error(%s)",
				__FILE__, __LINE__, myname,
				path, acl_last_serror());
			continue;
		}

		if (!S_ISDIR(sbuf.st_mode)) {
			ACL_SAFE_STRNCPY(scan->file_name, name,
				sizeof(scan->file_name));
			if (scan->scan_fn && scan->scan_fn(scan,
						scan->scan_ctx) < 0) {
				break;
			}

			scan->nfiles++;
			scan->nsize += sbuf.st_size;
			SANE_UNLINK(path);
			continue;
		}

		scan->file_name[0] = 0;
		if (SCAN_RECURSIVE(scan) && acl_scan_dir_push(scan, name) < 0) {
			continue;
		}

		if (scan->scan_fn && scan->scan_fn(scan, scan->scan_ctx) < 0) {
			break;
		}

	}

	if (ndir) {
		*ndir = scan->ndirs;
	}
	if (nfile) {
		*nfile = scan->nfiles;
	}

	return scan->nsize;
}

acl_int64 acl_scan_dir_rm(const char *pathname, int recursive,
	int *ndir, int *nfile)
{
	const char *myname = "acl_scan_dir_rmall";
	ACL_SCAN_DIR *scan;
	struct acl_stat sbuf;
	acl_int64 nsize;

	if (ndir) {
		*ndir = 0;
	}
	if (nfile) {
		*nfile = 0;
	}

	if (acl_stat(pathname, &sbuf) < 0) {
		acl_msg_error("%s(%d), %s: stat pathname(%s) error(%s)",
			__FILE__, __LINE__, myname, pathname,
			acl_last_serror());
		return -1;
	}
	if (S_ISDIR(sbuf.st_mode) == 0) {
		if (nfile) {
			*nfile = 1;
		}
		SANE_UNLINK(pathname);
		return 1;
	}

	scan = acl_scan_dir_open(pathname, recursive);
	if (scan == NULL) {
		acl_msg_error("%s(%d), %s: open path(%s) error(%s)",
			__FILE__, __LINE__, myname,
			pathname, acl_last_serror());
		return -1;
	}

	acl_scan_dir_rm2(scan, ndir, nfile);

	nsize = scan->nsize;
	acl_scan_dir_close(scan);
	return nsize;
}
