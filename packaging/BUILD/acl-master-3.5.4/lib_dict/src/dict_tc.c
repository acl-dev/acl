#include "StdAfx.h"

#ifdef HAS_TC

#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "debug_var.h"
#include "dict.h"
#include "dict_tc.h"

#include <tcutil.h>
#include <tcadb.h>

/* Application-specific. */

typedef struct {
	DICT    dict;			/* generic members */
	TCADB	*adb;			/* TCADB structure */
#define	DICT_TC_HDB	0
#define	DICT_TC_BDB	1
#define	DICT_TC_FDB	2
#define	DICT_TC_TDB	3
#define	DICT_TC_HMDB	4
#define	DICT_TC_BMDB	5
	int	type;
} DICT_TC;

/* dict_tc_error - get the error status */

static const char *dict_tc_error(DICT_TC *dict_tc, int *ecode)
{
	const char *myname = "dict_tc_error";
	const char *err = "unknown error";
	int   ret;

	switch (dict_tc->type) {
	case DICT_TC_HDB:
		if (dict_tc->adb->hdb == NULL)
			acl_msg_fatal("%s: hdb null", myname);
		ret = tchdbecode(dict_tc->adb->hdb);
		if (ecode)
			*ecode = ret;
		err = tchdberrmsg(ret);
		break;
	case DICT_TC_BDB:
		if (dict_tc->adb->bdb == NULL)
			acl_msg_fatal("%s: bdb null", myname);
		ret = tcbdbecode(dict_tc->adb->bdb);
		if (ecode)
			*ecode = ret;
		err = tcbdberrmsg(ret);
		break;
	case DICT_TC_FDB:
		if (dict_tc->adb->fdb == NULL)
			acl_msg_fatal("%s: fdb null", myname);
		ret = tcfdbecode(dict_tc->adb->fdb);
		if (ecode)
			*ecode = ret;
		err = tcfdberrmsg(ret);
		break;
	case DICT_TC_TDB:
		if (dict_tc->adb->tdb == NULL)
			acl_msg_fatal("%s: tdb null", myname);
		ret = tctdbecode(dict_tc->adb->tdb);
		if (ecode)
			*ecode = ret;
		err = tctdberrmsg(ret);
		break;
	case DICT_TC_HMDB:
		if (dict_tc->adb->mdb == NULL)
			acl_msg_fatal("%s: mdb null", myname);
	case DICT_TC_BMDB:
		if (dict_tc->adb->ndb == NULL)
			acl_msg_fatal("%s: ndb null", myname);
	default:
		if (ecode)
			*ecode = -1;
		break;
	}
	return (err);
}

/* dict_tc_lookup - find database entry */

static const char *dict_tc_lookup(DICT *dict, char *name, size_t name_len, char **value, size_t *size)
{
	DICT_TC *dict_tc = (DICT_TC *) dict;
	char *buf = NULL;
	char *result = NULL;

	dict_errno = 0;

	if (size)
		*size = 0;
	if (value == NULL)
		acl_msg_fatal("dict_db_lookup: value null");

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0)
			dict->fold_buf = acl_vstring_alloc(10);
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}

	/*
	 * See if this TC file was written with one null byte appended to key
	 * and value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		result = tcadbget(dict_tc->adb, name, (int) name_len + 1, (int*) size);
		if (result != NULL)
			dict->flags &= ~DICT_FLAG_TRY0NULL;
	}

	/*
	 * See if this TC file was written with no null byte appended to key and
	 * value.
	 */
	if (result == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
		result = tcadbget(dict_tc->adb, name, (int) name_len, (int*) size);
		if (result != NULL)
			dict->flags &= ~DICT_FLAG_TRY1NULL;
	}

	if (result) {
		buf = acl_mymalloc(*size + 1);
		memcpy(buf, result, *size);
		buf[*size] = '\0';
		free(result);  /* free the memory allocated by TC db */
		result = buf;
	}
	/* No locking so not release the lock.  */

	*value = result;
	return (result);
}

/* dict_tc_update - add database entry */

static void dict_tc_update(DICT *dict, char *name, size_t name_len, char *value, size_t size)
{
	const char *myname = "dict_tc_update";
	DICT_TC *dict_tc = (DICT_TC *) dict;
	int   ksize, vsize;
	const char *ptr;
	int   ecode = 0;

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0)
			dict->fold_buf = acl_vstring_alloc(10);
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}
	ksize = (int) name_len;
	vsize = (int) size;

	/*
	 * Optionally append a null byte to key and value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		ksize++;
		vsize++;
	}

	/*
	 * Do the add operation.  No locking is done.
	 */
	if (dict->flags & DICT_FLAG_DUP_IGNORE) {
		if (!tcadbputcat(dict_tc->adb, name, ksize, value, vsize)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_fatal("%s(%d): add error(%d, %s)",
				myname, __LINE__, ecode, ptr);
		}
	} else if (dict->flags & DICT_FLAG_DUP_WARN) {
		if (!tcadbput(dict_tc->adb, name, ksize, value, vsize)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_warn("%s(%d): add error(%d, %s)",
				myname, __LINE__, ecode, ptr);
		}
	} else {
#if 1
		if (!tcadbput(dict_tc->adb, name, ksize, value, vsize)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_fatal("%s(%d): add error(%d, %s)",
				myname, __LINE__, ecode, ptr);
		}
#else
		if (!tcadbputkeep(dict_tc->adb, name, ksize, value, vsize)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_fatal("%s(%d): add error(%d, %s)",
				myname, __LINE__, ecode, ptr);
		}
#endif
	}

	if (!ecode && (dict->flags & DICT_FLAG_SYNC_UPDATE)) {
		if (!tcadbsync(dict_tc->adb)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_warn("%s(%d): delete from %s error(%d, %s)",
				myname, __LINE__, dict_tc->dict.name, ecode, ptr);
		}
	}
}

/* dict_tc_delete - delete one entry from the dictionary */

static int dict_tc_delete(DICT *dict, char *name, size_t name_len)
{
	const char *myname = "dict_tc_delete";
	DICT_TC *dict_tc = (DICT_TC *) dict;
	int	ecode = 0;
	const char *ptr;

	/*
	 * Sanity check.
	 */
	if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
		acl_msg_panic("dict_db_delete: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0)
			dict->fold_buf = acl_vstring_alloc(10);
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}

	/*
	 * See if this DB file was written with one null byte appended to key and
	 * value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		if (!tcadbout(dict_tc->adb, name, (int) name_len + 1)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_warn("%s(%d): delete from %s error(%d, %s)",
				myname, __LINE__, dict_tc->dict.name, ecode, ptr);
		}
	}

	/*
	 * See if this DB file was written with no null byte appended to key and
	 * value.
	 */
	if (!ecode && (dict->flags & DICT_FLAG_TRY0NULL)) {
		if (!tcadbout(dict_tc->adb, name, (int) name_len)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_warn("%s(%d): delete from %s error(%d, %s)",
				myname, __LINE__, dict_tc->dict.name, ecode, ptr);
		}
	}

	if (!ecode && (dict->flags & DICT_FLAG_SYNC_UPDATE)) {
		if (!tcadbsync(dict_tc->adb)) {
			ptr = dict_tc_error(dict_tc, &ecode);
			acl_msg_warn("%s(%d): delete from %s error(%d, %s)",
				myname, __LINE__, dict_tc->dict.name, ecode, ptr);
		}
	}

	return (ecode);
}


/* dict_tc_close - close data base */

static void dict_tc_close(DICT *dict)
{
	const char *myname = "dict_tc_close";
	DICT_TC *dict_tc = (DICT_TC *) dict;
	const char *ptr;
	int   ecode;

	if (!tcadbclose(dict_tc->adb)) {
		ptr = dict_tc_error(dict_tc, &ecode);
		acl_msg_error("%s: close tc db error(%d, %s)",
			myname, ecode, ptr);
	}
	if (dict->fold_buf)
		acl_vstring_free(dict->fold_buf);
	dict_free(dict);
}

/* dict_tc_open - open data base */

static DICT *dict_tc_open2(const char *path, int dict_flags)
{
	const char *myname = "dict_tc_open2";
	DICT_TC *dict_tc;
	struct stat st;
	const char *ptr;

	dict_tc = (DICT_TC *) dict_alloc(DICT_TYPE_TC, path, sizeof(*dict_tc));

	dict_tc->adb = tcadbnew();

	ptr = strrchr(path, '.');
	if (strcmp(path, "*") == 0 || strncmp(path, "*#", 2) == 0) {
		dict_tc->type = DICT_TC_HMDB;
	} else if (strcmp(path, "+") == 0 || strncmp(path, "+#", 2) == 0) {
		dict_tc->type = DICT_TC_BMDB;
	} else if (ptr && strncmp(ptr, ".tch", 4) == 0) {
		dict_tc->type = DICT_TC_HDB;
	} else if (ptr && strncmp(ptr, ".tcb", 4) == 0) {
		dict_tc->type = DICT_TC_BDB;
	} else if (ptr && strncmp(ptr, ".tcf", 4) == 0) {
		dict_tc->type = DICT_TC_FDB;
	} else if (ptr && strncmp(ptr, ".tct", 4) == 0) {
		dict_tc->type = DICT_TC_TDB;
	} else {
		acl_msg_fatal("%s(%d): ext(%s) in path(%s) invalid",
			myname, __LINE__, ptr, path);
	}

	if(!tcadbopen(dict_tc->adb, path))
		acl_msg_fatal("%s: tcadbopen %s error", myname, path);

	dict_tc->dict.lookup = dict_tc_lookup;
	dict_tc->dict.update = dict_tc_update;
	dict_tc->dict.delete_it = dict_tc_delete;
	dict_tc->dict.close = dict_tc_close;

	dict_tc->dict.stat_fd = -1;

	/*
	dict_cdbq->dict.mtime = st.st_mtime;
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
	*/

	/*
	 * Warn if the source file is newer than the indexed file, except when
	 * the source file changed only seconds ago.
	 */
	if (stat(path, &st) == 0
			&& st.st_mtime > dict_tc->dict.mtime
			&& st.st_mtime < time((time_t *) 0) - 100)
		acl_msg_warn("database %s is older than source file %s", path, path);

	/*
	 * If undecided about appending a null byte to key and value, choose to
	 * try both in query mode.
	 */
	if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
		dict_flags |= DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL;
	dict_tc->dict.flags = dict_flags | DICT_FLAG_FIXED;
	if (dict_flags & DICT_FLAG_FOLD_FIX)
		dict_tc->dict.fold_buf = acl_vstring_alloc(10);

	return (&dict_tc->dict);
}

/* dict_tc_open - open data base */

DICT   *dict_tc_open(const char *path, int open_flags, int dict_flags)
{
	switch (open_flags & (O_RDONLY | O_RDWR | O_WRONLY | O_CREAT | O_TRUNC)) {
		case O_RDONLY:			/* query mode */
		case O_WRONLY | O_CREAT | O_TRUNC:		/* create mode */
		case O_RDWR | O_CREAT | O_TRUNC:		/* sloppiness */
			return dict_tc_open2(path, dict_flags);
		default:
			acl_msg_fatal("dict_cdb_open: inappropriate open flags for cdb database"
				" - specify O_RDONLY or O_WRONLY|O_CREAT|O_TRUNC");
	}

	/* not reached */
	return (NULL);
}

#endif					/* HAS_TC */
