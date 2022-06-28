#include "StdAfx.h"

#ifdef HAS_CDB
#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include "debug_var.h"
#include "dict.h"
#include "dict_cdb.h"

#include <cdb.h>
#ifndef TINYCDB_VERSION
#include <cdb_make.h>
#endif
#ifndef cdb_fileno
#define cdb_fileno(c) ((c)->fd)
#endif

#ifndef CDB_SUFFIX
#define CDB_SUFFIX ".cdb"
#endif
#ifndef CDB_TMP_SUFFIX
#define CDB_TMP_SUFFIX CDB_SUFFIX ".tmp"
#endif

/* Application-specific. */

typedef struct {
	DICT    dict;			/* generic members */
	struct cdb cdb;			/* cdb structure */
} DICT_CDBQ;				/* query interface */

typedef struct {
	DICT    dict;			/* generic members */
	struct cdb_make cdbm;		/* cdb_make structure */
	char   *cdb_path;		/* cdb pathname (.cdb) */
	char   *tmp_path;		/* temporary pathname (.tmp) */
} DICT_CDBM;				/* rebuild interface */

/* dict_cdbq_lookup - find database entry, query mode */

static const char *dict_cdbq_lookup(DICT *dict, char *name, size_t name_len,
	char **value, size_t *size)
{
	DICT_CDBQ *dict_cdbq = (DICT_CDBQ *) dict;
	unsigned vlen;
	int     status = 0;
	char *buf = NULL;
	char *result = NULL;

	dict_errno = 0;

	if (size) {
		*size = 0;
	}
	if (value == NULL) {
		acl_msg_fatal("dict_db_lookup: value null");
	}

	/* CDB is constant, so do not try to acquire a lock. */

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0) {
			dict->fold_buf = acl_vstring_alloc(10);
		}
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}

	/*
	 * See if this CDB file was written with one null byte appended to key
	 * and value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		status = cdb_find(&dict_cdbq->cdb, name, name_len + 1);
		if (status > 0) {
			dict->flags &= ~DICT_FLAG_TRY0NULL;
		}
	}

	/*
	 * See if this CDB file was written with no null byte appended to key and
	 * value.
	 */
	if (status == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
		status = cdb_find(&dict_cdbq->cdb, name, name_len);
		if (status > 0) {
			dict->flags &= ~DICT_FLAG_TRY1NULL;
		}
	}
	if (status < 0) {
		acl_msg_fatal("error reading %s: %s", dict->name, acl_last_serror());
	}

	if (status) {
		vlen = cdb_datalen(&dict_cdbq->cdb);
		buf = acl_mymalloc(vlen + 1);
		if (cdb_read(&dict_cdbq->cdb, buf, vlen,
			cdb_datapos(&dict_cdbq->cdb)) < 0) {
			acl_msg_fatal("error reading %s: %s",
				dict->name, acl_last_serror());
		}
		buf[vlen] = '\0';
		result = buf;
	}
	/* No locking so not release the lock.  */

	*value = result;
	return (result);
}

/* dict_cdbq_close - close data base, query mode */

static void dict_cdbq_close(DICT *dict)
{
	DICT_CDBQ *dict_cdbq = (DICT_CDBQ *) dict;

	cdb_free(&dict_cdbq->cdb);
	close(dict->stat_fd);
	if (dict->fold_buf) {
		acl_vstring_free(dict->fold_buf);
	}
	dict_free(dict);
}

/* dict_cdbq_open - open data base, query mode */

static DICT *dict_cdbq_open(const char *path, int dict_flags)
{
	DICT_CDBQ *dict_cdbq;
	struct stat st;
	char   *cdb_path;
	int     fd;

	cdb_path = acl_concatenate(path, CDB_SUFFIX, (char *) 0);

	if ((fd = open(cdb_path, O_RDONLY)) < 0) {
		acl_msg_fatal("%s(%d), %s: open database %s: %s",
			__FILE__, __LINE__, __FUNCTION__, cdb_path,
			acl_last_serror());
	}

	dict_cdbq = (DICT_CDBQ *) dict_alloc(DICT_TYPE_CDB,
			cdb_path, sizeof(*dict_cdbq));
#if defined(TINYCDB_VERSION)
	if (cdb_init(&(dict_cdbq->cdb), fd) != 0) {
		acl_msg_fatal("dict_cdbq_open: unable to init %s: %s",
			cdb_path, acl_last_serror());
	}
#else
	cdb_init(&(dict_cdbq->cdb), fd);
#endif
	dict_cdbq->dict.lookup = dict_cdbq_lookup;
	dict_cdbq->dict.close = dict_cdbq_close;
	dict_cdbq->dict.stat_fd = fd;
	if (fstat(fd, &st) < 0) {
		acl_msg_fatal("dict_dbq_open: fstat: %s", acl_last_serror());
	}
	dict_cdbq->dict.mtime = st.st_mtime;
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	/*
	 * Warn if the source file is newer than the indexed file, except when
	 * the source file changed only seconds ago.
	 */
	if (stat(path, &st) == 0
		&& st.st_mtime > dict_cdbq->dict.mtime
		&& st.st_mtime < time((time_t *) 0) - 100) {
		acl_msg_warn("database %s is older than source file %s",
			cdb_path, path);
	}

	/*
	 * If undecided about appending a null byte to key and value, choose to
	 * try both in query mode.
	 */
	if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0) {
		dict_flags |= DICT_FLAG_TRY0NULL | DICT_FLAG_TRY1NULL;
	}
	dict_cdbq->dict.flags = dict_flags | DICT_FLAG_FIXED;
	if (dict_flags & DICT_FLAG_FOLD_FIX) {
		dict_cdbq->dict.fold_buf = acl_vstring_alloc(10);
	}

	acl_myfree(cdb_path);
	return &dict_cdbq->dict;
}

/* dict_cdbm_update - add database entry, create mode */

static void dict_cdbm_update(DICT *dict, char *name, size_t name_len,
	char *value, size_t size)
{
	DICT_CDBM *dict_cdbm = (DICT_CDBM *) dict;
	unsigned ksize, vsize;
#ifdef TINYCDB_VERSION
	int     r;
#endif

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0) {
			dict->fold_buf = acl_vstring_alloc(10);
		}
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}
	ksize = name_len;
	vsize = size;

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
#ifdef TINYCDB_VERSION
#ifndef CDB_PUT_ADD
#error please upgrate tinycdb to at least 0.5 version
#endif
	if (dict->flags & DICT_FLAG_DUP_IGNORE) {
		r = CDB_PUT_ADD;
	} else if (dict->flags & DICT_FLAG_DUP_REPLACE) {
		r = CDB_PUT_REPLACE;
	} else {
		r = CDB_PUT_INSERT;
	}

	r = cdb_make_put(&dict_cdbm->cdbm, name, ksize, value, vsize, r);
	if (r < 0) {
		acl_msg_fatal("error writing %s: %s",
			dict_cdbm->tmp_path, acl_last_serror());
	} else if (r > 0) {
		if (dict->flags & (DICT_FLAG_DUP_IGNORE | DICT_FLAG_DUP_REPLACE)) {
			/* void */ ;
		} else if (dict->flags & DICT_FLAG_DUP_WARN) {
			acl_msg_warn("%s: duplicate entry: \"%s\"",
				dict_cdbm->dict.name, name);
		} else {
			acl_msg_fatal("%s: duplicate entry: \"%s\"",
				dict_cdbm->dict.name, name);
		}
	}
#else
	if (cdb_make_add(&dict_cdbm->cdbm, name, ksize, value, vsize) < 0) {
		acl_msg_fatal("error writing %s: %s",
			dict_cdbm->tmp_path, acl_last_serror());
	}
#endif
}

/* dict_cdbm_close - close data base and rename file.tmp to file.cdb */

static void dict_cdbm_close(DICT *dict)
{
	DICT_CDBM *dict_cdbm = (DICT_CDBM *) dict;
	int     fd = cdb_fileno(&dict_cdbm->cdbm);

	/*
	 * Note: if FCNTL locking is used, closing any file descriptor on a
	 * locked file cancels all locks that the process may have on that file.
	 * CDB is FCNTL locking safe, because it uses the same file descriptor
	 * for database I/O and locking.
	 */
	if (cdb_make_finish(&dict_cdbm->cdbm) < 0) {
		acl_msg_fatal("finish database %s: %s",
			dict_cdbm->tmp_path, acl_last_serror());
	}

	if (strcasecmp(dict_cdbm->tmp_path, dict_cdbm->cdb_path) != 0) {
		if (rename(dict_cdbm->tmp_path, dict_cdbm->cdb_path) < 0) {
			acl_msg_fatal("rename database from %s to %s: %s",
				dict_cdbm->tmp_path, dict_cdbm->cdb_path,
				acl_last_serror());
		}
	}

	if (close(fd) < 0) {				/* releases a lock */
		acl_msg_fatal("close database %s: %s",
			dict_cdbm->cdb_path, acl_last_serror());
	}
	acl_myfree(dict_cdbm->cdb_path);
	acl_myfree(dict_cdbm->tmp_path);
	if (dict->fold_buf) {
		acl_vstring_free(dict->fold_buf);
	}
	dict_free(dict);
}

/* dict_cdbm_open - create database as file.tmp */

static DICT *dict_cdbm_open(const char *path, int dict_flags)
{
	DICT_CDBM *dict_cdbm;
	char   *cdb_path;
	char   *tmp_path;
	int     fd;
	struct stat st0, st1;

	cdb_path = acl_concatenate(path, CDB_SUFFIX, (char *) 0);
#if 0
	tmp_path = acl_concatenate(path, CDB_SUFFIX, (char *) 0);
#else
	tmp_path = acl_concatenate(path, CDB_TMP_SUFFIX, (char *) 0);
#endif

	/*
	 * Repeat until we have opened *and* locked *existing* file. Since the
	 * new (tmp) file will be renamed to be .cdb file, locking here is
	 * somewhat funny to work around possible race conditions.  Note that we
	 * can't open a file with O_TRUNC as we can't know if another process
	 * isn't creating it at the same time.
	 */
	for (;;) {
		if ((fd = open(tmp_path, O_RDWR | O_CREAT, 0644)) < 0
			|| fstat(fd, &st0) < 0) {
			acl_msg_fatal("%s(%d), %s: open database %s: %s",
				__FILE__, __LINE__, __FUNCTION__,
				tmp_path, acl_last_serror());
		}

		/*
		 * Get an exclusive lock - we're going to change the database
		 * so we can't have any spectators.
		 */
		if (acl_myflock(fd, ACL_INTERNAL_LOCK, ACL_FLOCK_OP_EXCLUSIVE) < 0) {
			acl_msg_fatal("lock %s: %s", tmp_path, acl_last_serror());
		}

		if (stat(tmp_path, &st1) < 0) {
			acl_msg_fatal("stat(%s): %s", tmp_path, acl_last_serror());
		}

		/*
		 * Compare file's state before and after lock: should be the same,
		 * and nlinks should be >0, or else we opened non-existing file...
		 */
		if (st0.st_ino == st1.st_ino && st0.st_dev == st1.st_dev
			&& st0.st_rdev == st1.st_rdev
			&& st0.st_nlink == st1.st_nlink
			&& st0.st_nlink > 0) {
			break;				/* successefully opened */
		}

		close(fd);
	}

#ifndef NO_FTRUNCATE
	if (st0.st_size) {
		ftruncate(fd, 0);
	}
#endif

	dict_cdbm = (DICT_CDBM *) dict_alloc(DICT_TYPE_CDB, path,
			sizeof(*dict_cdbm));
	if (cdb_make_start(&dict_cdbm->cdbm, fd) < 0) {
		acl_msg_fatal("initialize database %s: %s",
			tmp_path, acl_last_serror());
	}
	dict_cdbm->dict.close = dict_cdbm_close;
	dict_cdbm->dict.update = dict_cdbm_update;
	dict_cdbm->cdb_path = cdb_path;
	dict_cdbm->tmp_path = tmp_path;
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	/*
	 * If undecided about appending a null byte to key and value, choose a
	 * default to not append a null byte when creating a cdb.
	 */
	if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0) {
		dict_flags |= DICT_FLAG_TRY0NULL;
	} else if ((dict_flags & DICT_FLAG_TRY1NULL)
		&& (dict_flags & DICT_FLAG_TRY0NULL)) {
		dict_flags &= ~DICT_FLAG_TRY0NULL;
	}
	dict_cdbm->dict.flags = dict_flags | DICT_FLAG_FIXED;
	if (dict_flags & DICT_FLAG_FOLD_FIX) {
		dict_cdbm->dict.fold_buf = acl_vstring_alloc(10);
	}

	return &dict_cdbm->dict;
}

/* dict_cdb_open - open data base for query mode or create mode */

DICT   *dict_cdb_open(const char *path, int open_flags, int dict_flags)
{
	switch (open_flags & (O_RDONLY | O_RDWR | O_WRONLY | O_CREAT | O_TRUNC)) {
	case O_RDONLY:					/* query mode */
		return dict_cdbq_open(path, dict_flags);
	case O_WRONLY | O_CREAT | O_TRUNC:		/* create mode */
	case O_RDWR | O_CREAT | O_TRUNC:		/* sloppiness */
		return dict_cdbm_open(path, dict_flags);
	default:
		acl_msg_fatal("dict_cdb_open: inappropriate open flags for"
			" cdb database - specify O_RDONLY or O_WRONLY|O_CREAT|O_TRUNC");
	}

	/* not reached */
	return NULL;
}

#endif					/* HAS_CDB */
