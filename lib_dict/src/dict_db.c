#include "StdAfx.h"
#include "dict.h"

/*
 * You can override the default dict_db_cache_size setting before calling
 * dict_hash_open() or dict_btree_open(). This is done in mkmap_db_open() to
 * set a larger memory pool for database (re)builds.
 * 
 * XXX This should be specified via the DICT interface so that it becomes an
 * object property, instead of being specified by poking a global variable
 * so that it becomes a class property.
 */
int dict_db_cache_size = (1024 * 1024 * 1);	/* default memory pool */
int dict_db_page_size = 0;			/* default same as the system pagesize */

#ifdef HAS_BDB
#include "debug_var.h"

#ifdef WIN32
# include "db.h"
# include <direct.h>
# define getcwd _getcwd
#else
# include "db.h"
#endif

/* System library. */

#include <sys/stat.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#ifdef ACL_UNIX
# include <unistd.h>
#endif
#include <errno.h>

#if defined(_DB_185_H_) && defined(USE_FCNTL_LOCK)
#error "Error: this system must not use the db 1.85 compatibility interface"
#endif

#ifndef DB_VERSION_MAJOR
#define DB_VERSION_MAJOR 1
#define DICT_DB_GET(db, key, val, flag)	db->get(db, key, val, flag)
#define DICT_DB_PUT(db, key, val, flag)	db->put(db, key, val, flag)
#define DICT_DB_DEL(db, key, flag)	db->del(db, key, flag)
#define DICT_DB_SYNC(db, flag)		db->sync(db, flag)
#define DICT_DB_CLOSE(db)		db->close(db)
#define DONT_CLOBBER			R_NOOVERWRITE
#endif

#if DB_VERSION_MAJOR > 1
#define DICT_DB_GET(db, key, val, flag)	sanitize(db->get(db, 0, key, val, flag))
#define DICT_DB_PUT(db, tid, key, val, flag)	sanitize(db->put(db, tid, key, val, flag))
#define DICT_DB_DEL(db, key, flag)	sanitize(db->del(db, 0, key, flag))
#define DICT_DB_SYNC(db, flag)		((errno = db->sync(db, flag)) ? -1 : 0)
#define DICT_DB_CLOSE(db)		((errno = db->close(db, 0)) ? -1 : 0)
#define DONT_CLOBBER			DB_NOOVERWRITE
#endif

#if (DB_VERSION_MAJOR == 2 && DB_VERSION_MINOR < 6)
#define DICT_DB_CURSOR(db, curs)	(db)->cursor((db), NULL, (curs))
#else
#define DICT_DB_CURSOR(db, curs)	(db)->cursor((db), NULL, (curs), 0)
#endif

#ifndef DB_FCNTL_LOCKING
#define DB_FCNTL_LOCKING		0
#endif

#include "dict.h"
#include "dict_db.h"

/* Application-specific. */

typedef struct {
	DICT    dict;               /* generic members */
	DB     *db;                 /* open db file */
#if DB_VERSION_MAJOR > 1
	DBC    *cursor;             /* dict_db_sequence() */
	DB_ENV *dbenv;
	int     quit;
#endif
} DICT_DB;

#define DICT_DB_NELM		(1024 * 1024 * 10)

__thread char    ebuf[256];

static int compare_key(DB *dbp acl_unused, const DBT *a, const DBT *b);

int (*dict_db_cmpkey_fn)(DB*, const DBT*, const DBT*) = compare_key;

/*----------------------------------------------------------------------------*/

static void *dict_mem_malloc(size_t size)
{
	return (acl_mymalloc(size));
}

static void *dict_mem_realloc(void *ptr, size_t size)
{
	return (acl_myrealloc(ptr, size));
}

static void dict_mem_free(void *ptr)
{
	acl_myfree(ptr);
}

#if DB_VERSION_MAJOR > 1

/* sanitize - sanitize db_get/put/del result */

static int sanitize(int status)
{
	/*
	 * XXX This is unclean but avoids a lot of clutter elsewhere. Categorize
	 * results into non-fatal errors (i.e., errors that we can deal with),
	 * success, or fatal error (i.e., all other errors).
	 */
	switch (status) {
		case DB_NOTFOUND:       /* get, del */
		case DB_KEYEXIST:       /* put */
			errno = status;
			return (1);         /* non-fatal */
		case 0:
			return (0);         /* success */

		case DB_KEYEMPTY:       /* get, others? */
			status = EINVAL;
			/* FALLTHROUGH */
		default:
			errno = status;
			return (-1);    /* fatal */
	}
}

#endif

/* dict_db_lookup - find database entry */

static const char *dict_db_lookup(DICT *dict, char *name, size_t name_len, char **value, size_t *size)
{
	DICT_DB *dict_db = (DICT_DB *) dict;
	DB     *db = dict_db->db;
	DBT     db_key;
	DBT     db_value;
	int     status;
	char *result = NULL;

	/*
	 * Sanity check.
	 */
	if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
		acl_msg_panic("dict_db_lookup: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

	if (size)
		*size = 0;
	if (value == NULL)
		acl_msg_fatal("dict_db_lookup: value null");

	dict_errno = 0;
	memset(&db_key, 0, sizeof(db_key));
	memset(&db_value, 0, sizeof(db_value));
	db_value.flags = DB_DBT_MALLOC;

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
	 * Acquire a shared lock.
	 */

	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_SHARED) < 0)
		acl_msg_fatal("%s: lock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	/*
	 * See if this DB file was written with one null byte appended to key and
	 * value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		db_key.data = (void *) name;
		db_key.size = (int) name_len + 1;
		if ((status = DICT_DB_GET(db, &db_key, &db_value, 0)) < 0)
			acl_msg_fatal("error reading %s: %s", dict_db->dict.name,
				db_strerror(errno));
		if (status == 0) {
			dict->flags &= ~DICT_FLAG_TRY0NULL;
			if (size)
				*size = db_value.size;
			result = db_value.data;
		}
	}

	/*
	 * See if this DB file was written with no null byte appended to key and
	 * value.
	 */
	if (result == 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
		db_key.data = (void *) name;
		db_key.size = (int) name_len;
		if ((status = DICT_DB_GET(db, &db_key, &db_value, 0)) < 0)
			acl_msg_fatal("error reading %s: %s", dict_db->dict.name,
				db_strerror(errno));
		if (status == 0) {
			dict->flags &= ~DICT_FLAG_TRY1NULL;
			if (size)
				*size = db_value.size;
			result = db_value.data;
		}
	}

	/*
	 * Release the shared lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
	    && acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_NONE) < 0)
		acl_msg_fatal("%s: unlock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	*value = result;
	return (result);
}

/* dict_db_update - add or update database entry */

static void dict_db_update(DICT *dict, char *name, size_t name_len, char *value, size_t len)
{
	const char *myname = "dict_db_update";
	DICT_DB *dict_db = (DICT_DB *) dict;
	DB     *db = dict_db->db;
	DBT     db_key;
	DBT     db_value;
	DB_TXN *tid = NULL;
	int     status;

	/*
	 * Sanity check.
	 */
	if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
		acl_msg_panic("%s(%d): dict_db_update: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag",
			myname, __LINE__);

	/*
	 * Avoid deadlock
	 */
	if (dict_db->cursor) {
		dict_db->cursor->c_close(dict_db->cursor);
		dict_db->cursor = NULL;
	}

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0)
			dict->fold_buf = acl_vstring_alloc(10);
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}
	memset(&db_key, 0, sizeof(db_key));
	memset(&db_value, 0, sizeof(db_value));
	db_key.data = (void *) name;
	db_key.size = (int) name_len;
	db_value.data = (void *) value;
	db_value.size = (int) len;

	/*
	 * If undecided about appending a null byte to key and value, choose a
	 * default depending on the platform.
	 */
	if ((dict->flags & DICT_FLAG_TRY1NULL)
		&& (dict->flags & DICT_FLAG_TRY0NULL)) {
#ifdef DB_NO_TRAILING_NULL
		dict->flags &= ~DICT_FLAG_TRY1NULL;
#else
		dict->flags &= ~DICT_FLAG_TRY0NULL;
#endif
	}

	/*
	 * Optionally append a null byte to key and value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		db_key.size++;
		db_value.size++;
	}

	/*
	 * Acquire an exclusive lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_EXCLUSIVE) < 0)
		acl_msg_fatal("%s(%d): %s lock dictionary error %s",
			myname, __LINE__, dict_db->dict.name, acl_last_strerror(ebuf, sizeof(ebuf)));

	/*
	 * Do the update.
	 */

	while (1) {
		/* Begin the transaction. */
		if (0 && (status = dict_db->dbenv->txn_begin(dict_db->dbenv, NULL, &tid, 0)) != 0)
			acl_msg_fatal("txn_begin: %s", db_strerror(status));
		if ((status = DICT_DB_PUT(db, tid, &db_key, &db_value,
			(dict->flags & DICT_FLAG_DUP_REPLACE) ? 0 : DONT_CLOBBER)) != 0) {
			acl_msg_warn("%s(%d): (%u) writing %s: %s", myname, __LINE__,
				(unsigned) acl_pthread_self(), dict_db->dict.name, db_strerror(errno));
			if (errno != DB_LOCK_DEADLOCK)
				break;
			if (tid && (status = tid->abort(tid)) != 0)
				acl_msg_fatal("%s(%d): DB_TXN->abort error(%s)",
					myname, __LINE__, db_strerror(status));
		} else
			break;
	}

	if (status) {
		if (dict->flags & DICT_FLAG_DUP_IGNORE) {
			/* void */ ;
		} else if (dict->flags & DICT_FLAG_DUP_WARN) {
			acl_msg_warn("%s(%d): %s: duplicate entry: \"%s\"",
				myname, __LINE__, dict_db->dict.name, name);
		} else {
			acl_msg_fatal("%s(%d): %s: duplicate entry: \"%s\", %s",
				myname, __LINE__, dict_db->dict.name, name, db_strerror(errno));
		}
	}

	if (dict->flags & DICT_FLAG_SYNC_UPDATE) {
		while (1) {
			if ((status = DICT_DB_SYNC(db, 0)) != 0) {
				acl_msg_warn("%s(%d): %s: flush dictionary: %s",
					myname, __LINE__, dict_db->dict.name, db_strerror(errno));
				if (errno != DB_LOCK_DEADLOCK)
					break;
			} else
				break;
		}
	}

	if (tid && (status = tid->commit(tid, 0)) != 0) {
		acl_msg_fatal("%s(%d): commit error(%s)",
			myname, __LINE__, db_strerror(status));
	}

	/*
	 * Release the exclusive lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_NONE) < 0)
		acl_msg_fatal("%s(%d): %s: unlock dictionary: %s", myname, __LINE__,
			dict_db->dict.name, acl_last_strerror(ebuf, sizeof(ebuf)));
}

/* delete one entry from the dictionary */

static int dict_db_delete(DICT *dict, char *name, size_t name_len)
{
	DICT_DB *dict_db = (DICT_DB *) dict;
	DB     *db = dict_db->db;
	DBT     db_key;
	int     status = 1;
	int     flags = 0;

	/*
	 * Sanity check.
	 */
	if ((dict->flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
		acl_msg_panic("dict_db_delete: no DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL flag");

	/*
	 * Avoid deadlock
	 */
	if (dict_db->cursor) {
		dict_db->cursor->c_close(dict_db->cursor);
		dict_db->cursor = NULL;
	}

	/*
	 * Optionally fold the key.
	 */
	if (dict->flags & DICT_FLAG_FOLD_FIX) {
		if (dict->fold_buf == 0)
			dict->fold_buf = acl_vstring_alloc(10);
		acl_vstring_strcpy(dict->fold_buf, name);
		name = acl_lowercase(acl_vstring_str(dict->fold_buf));
	}
	memset(&db_key, 0, sizeof(db_key));

	/*
	 * Acquire an exclusive lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_EXCLUSIVE) < 0)
		acl_msg_fatal("%s: lock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	/*
	 * See if this DB file was written with one null byte appended to key and
	 * value.
	 */
	if (dict->flags & DICT_FLAG_TRY1NULL) {
		db_key.data = (void *) name;
		db_key.size = (int) name_len + 1;
		while (1) {
			if ((status = DICT_DB_DEL(db, &db_key, flags)) != 0)
				acl_msg_warn("deleting from %s: %s", dict_db->dict.name,
					db_strerror(errno));
			if (status == 0) {
				dict->flags &= ~DICT_FLAG_TRY0NULL;
				break;
			}
		}
	}

	/*
	 * See if this DB file was written with no null byte appended to key and
	 * value.
	 */
	if (status > 0 && (dict->flags & DICT_FLAG_TRY0NULL)) {
		db_key.data = (void *) name;
		db_key.size = (int) name_len;
		while (1) {
			if ((status = DICT_DB_DEL(db, &db_key, flags)) != 0)
				acl_msg_warn("deleting from %s: %s", dict_db->dict.name,
					acl_last_strerror(ebuf, sizeof(ebuf)));
			if (status == 0) {
				dict->flags &= ~DICT_FLAG_TRY1NULL;
				break;
			}
		}
	}
	if (dict->flags & DICT_FLAG_SYNC_UPDATE) {
		while (1) {
			if ((status = DICT_DB_SYNC(db, 0)) != 0)
				acl_msg_fatal("%s: flush dictionary: %s", dict_db->dict.name,
					db_strerror(errno));
			else
				break;
		}
	}

	/*
	 * Release the exclusive lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_NONE) < 0)
		acl_msg_fatal("%s: unlock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	return status;
}

/* dict_db_sequence_reset */

static void dict_db_sequence_reset(DICT *dict)
{
	DICT_DB *dict_db = (DICT_DB *) dict;

	if (dict_db->cursor) {
		dict_db->cursor->c_close(dict_db->cursor);
		dict_db->cursor = NULL;
	}
}

static int dict_db_sequence_delcur(DICT *dict)
{
	DICT_DB *dict_db = (DICT_DB *) dict;

	if (dict_db->cursor) {
		return (dict_db->cursor->c_del(dict_db->cursor, 0));
	}
	return (-1);
}

/* dict_db_sequence - traverse the dictionary */

static int dict_db_sequence(DICT *dict, int function,
	char **key, size_t *key_size, char **value, size_t *value_size)
{
	const char *myname = "dict_db_sequence";
	DICT_DB *dict_db = (DICT_DB *) dict;
	DB     *db = dict_db->db;
	DBT     db_key;
	DBT     db_value;
	int     status = 0;
	int     db_function = 0;

	*key = NULL;
	if (key_size)
		*key_size = 0;
	*value = NULL;
	if (value_size)
		*value_size = 0;

#if DB_VERSION_MAJOR > 1

	/*
	 * Initialize.
	 */

	dict_errno = 0;
	memset(&db_key, 0, sizeof(db_key));
	memset(&db_value, 0, sizeof(db_value));
	db_key.flags = DB_DBT_MALLOC;
	db_value.flags = DB_DBT_MALLOC;

	/*
	 * Determine the function.
	 */
	switch (function) {
		case DICT_SEQ_FUN_FIRST:
			if (dict_db->cursor == 0)
				DICT_DB_CURSOR(db, &(dict_db->cursor));
			db_function = DB_FIRST;
			break;
		case DICT_SEQ_FUN_NEXT:
			if (dict_db->cursor == 0) {
				/* If the cursor is not yet initialized,
				 * DB_NEXT is identical to DB_FIRST--see BDB help
				 */
				DICT_DB_CURSOR(db, &(dict_db->cursor));
				/* acl_msg_panic("%s: no cursor", myname); */
			}
			db_function = DB_NEXT;
			break;
		default:
			acl_msg_panic("%s: invalid function %d", myname, function);
	}

	/*
	 * Acquire a shared lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_SHARED) < 0)
		acl_msg_fatal("%s: lock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	/*
	 * Database lookup.
	 */
	status = dict_db->cursor->c_get(dict_db->cursor, &db_key, &db_value, db_function);
	if (status != 0 && status != DB_NOTFOUND)
		acl_msg_fatal("error [%d] seeking %s: %s", status, dict_db->dict.name,
			db_strerror(errno));

	/*
	 * Release the shared lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_NONE) < 0)
		acl_msg_fatal("%s: unlock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	if (status == 0) {
		/*
		 * Copy the result so it is guaranteed null terminated.
		 */
		*key = db_key.data;
		if (key_size)
			*key_size = db_key.size;
		*value = db_value.data;
		if (value_size)
			*value_size = db_value.size;
	} else {
		/*
		 * Avoid deadlock in update and delete functions
		 */
		if (dict_db->cursor) {
			dict_db->cursor->c_close(dict_db->cursor);
			dict_db->cursor = NULL;
		}

	}
	return (status);
#else

	/*
	 * determine the function
	 */
	switch (function) {
		case DICT_SEQ_FUN_FIRST:
			db_function = R_FIRST;
			break;
		case DICT_SEQ_FUN_NEXT:
			db_function = R_NEXT;
			break;
		default:
			acl_msg_panic("%s: invalid function %d", myname, function);
	}

	/*
	 * Acquire a shared lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& acl_myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_SHARED) < 0)
		acl_msg_fatal("%s: lock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	if ((status = db->seq(db, &db_key, &db_value, db_function)) < 0)
		acl_msg_fatal("error seeking %s: %s", dict_db->dict.name,
			db_strerror(errno));

	/*
	 * Release the shared lock.
	 */
	if ((dict->flags & DICT_FLAG_LOCK)
		&& myflock(dict->lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_NONE) < 0)
		acl_msg_fatal("%s: unlock dictionary: %s", dict_db->dict.name,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	if (status == 0) {
		/*
		 * Copy the result so that it is guaranteed null terminated.
		 */
		*key = db_key.data;
		if (key_size)
			*key_size = db_key.size;
		*value = db_value.data;
		if (value_size)
			*value_size = db_value.size;
	}
	return status;
#endif
}

/* dict_db_close - close data base */

static void dict_db_close(DICT *dict)
{
	DICT_DB *dict_db = (DICT_DB *) dict;
	int   status;

#if DB_VERSION_MAJOR > 1
	if (dict_db->cursor)
		dict_db->cursor->c_close(dict_db->cursor);
#endif
	if ((status = DICT_DB_SYNC(dict_db->db, 0)) != 0)
		acl_msg_fatal("flush database %s: %s", dict_db->dict.name,
			db_strerror(errno));
	if ((status = DICT_DB_CLOSE(dict_db->db)) != 0)
		acl_msg_fatal("close database %s: %s", dict_db->dict.name,
			db_strerror(errno));
#if DB_VERSION_MAJOR > 2
	if (dict_db->dbenv) {
		dict_db->quit = 1;
		while (1) {
			if (dict_db->quit == 2)
				break;
			sleep(1);
		}
		acl_msg_info("begin to sync db...");
		(void) dict_db->dbenv->memp_sync(dict_db->dbenv, NULL);
		acl_msg_info("sync db ok.");
		acl_msg_info("begin to close db...");
		(void) dict_db->dbenv->close(dict_db->dbenv, 0);
		acl_msg_info("close db ok.");
	}
#endif
	if (dict->fold_buf)
		acl_vstring_free(dict->fold_buf);
	dict_free(dict);
}

/* check_lock_thread - check deadlock for multi-threads */

static void *check_lock_thread(void *arg)
{
	DICT_DB *dict_db = (DICT_DB*) arg;

	while (!dict_db->quit) {
		sleep(1);
		(void) dict_db->dbenv->lock_detect(dict_db->dbenv, 0, DB_LOCK_YOUNGEST, NULL);
	}

	return (NULL);
}

/* memp_trickle_thread - put data from mem to disk */

static void *memp_trickle_thread(void *arg)
{
	DICT_DB *dict_db = (DICT_DB*) arg;
	int   wrote;

	while (!dict_db->quit) {
		sleep(1);
		(void) dict_db->dbenv->memp_trickle(dict_db->dbenv, 100, &wrote);
	}

	dict_db->quit = 2;
	return (NULL);
}

/* create_backend_threads - open two backend threads */

static void create_backend_threads(DICT_DB* dict_db)
{
	acl_pthread_attr_t attr;
	acl_pthread_t tid;

	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, 1);
	acl_pthread_create(&tid, &attr, check_lock_thread, dict_db);
	acl_pthread_create(&tid, &attr, memp_trickle_thread, dict_db);
}

/* dict_db_open - open data base */

static int compare_key(DB *dbp acl_unused, const DBT *a, const DBT *b)
{
	size_t n = a->size > b->size ? b->size : b->size;
	int   ret = memcmp(a->data,  b->data, n);

	if (ret != 0)
		return (ret);
	return (a->size - b->size);
}

static DICT *dict_db_open(const char *class, const char *path, int open_flags,
		int type, void *tweak acl_unused, int dict_flags)
{
	const char *myname = "dict_db_open";
	DICT_DB *dict_db;
	struct acl_stat st;
	DB     *db;
	char   *saved_path = acl_mystrdup(path);  /* [path/]dict_name */
	char   *db_root, *db_home, *db_name, *db_path;
	ACL_FILE_HANDLE lock_fd = ACL_FILE_INVALID;
	int     dbfd;

#if DB_VERSION_MAJOR > 1
	int     db_flags, env_flags = 0, size;
#endif
#if DB_VERSION_MAJOR > 2
	DB_ENV *dbenv;
#endif

	/*
	 * Mismatches between #include file and library are a common cause for
	 * trouble.
	 */
#if DB_VERSION_MAJOR > 1
	int     major_version;
	int     minor_version;
	int     patch_version;

	(void) db_version(&major_version, &minor_version, &patch_version);
	if (major_version != DB_VERSION_MAJOR || minor_version != DB_VERSION_MINOR)
		acl_msg_fatal("%s(%d): incorrect version of Berkeley DB: "
			"compiled against %d.%d.%d, run-time linked against %d.%d.%d",
			myname, __LINE__,
			DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH,
			major_version, minor_version, patch_version);
	if (acl_do_debug(DEBUG_DICT_DB, 1)) {
		acl_msg_info("Compiled against Berkeley DB: %d.%d.%d\n",
			DB_VERSION_MAJOR, DB_VERSION_MINOR, DB_VERSION_PATCH);
		acl_msg_info("Run-time linked against Berkeley DB: %d.%d.%d\n",
			major_version, minor_version, patch_version);
	}
#else
	acl_debug(DEBUG_DICT_DB, 1) ("Compiled against Berkeley DB version 1");
#endif

	/* saved_path: [path/]db_name */

	db_name = strrchr(saved_path, '/');
	if (db_name == NULL) {
		db_root = acl_mystrdup("db");
		db_home = acl_concatenate(db_root, "/", saved_path, ".d", (char*) 0);
		db_name = acl_concatenate(saved_path, ".db", (char*) 0);
		db_path = acl_concatenate(db_home, "/", db_name, (char*) 0);
	} else {
		*db_name++ = 0;
		db_root = acl_mystrdup(saved_path);
		db_home = acl_concatenate(db_root, "/", db_name, ".d", (char*) 0);
		db_name = acl_concatenate(db_name, ".db", (char*) 0);
		db_path = acl_concatenate(db_home, "/", db_name, (char*) 0);
	}

	acl_make_dirs(db_home, 0700);

	/*
	 * Note: DICT_FLAG_LOCK is used only by programs that do fine-grained (in
	 * the time domain) locking while accessing individual database records.
	 * 
	 * Programs such as postmap/postalias use their own large-grained (in the
	 * time domain) locks while rewriting the entire file.
	 * 
	 * XXX DB version 4.1 will not open a zero-length file. This means we must
	 * open an existing file without O_CREAT|O_TRUNC, and that we must let
	 * db_open() create a non-existent file for us.
	 */
#define LOCK_OPEN_FLAGS(f) ((f) & ~(O_CREAT|O_TRUNC))

	if (dict_flags & DICT_FLAG_LOCK) {
		if ((lock_fd = acl_file_open(db_path, LOCK_OPEN_FLAGS(open_flags), 0644)) == ACL_FILE_INVALID) {
			if (errno != ENOENT) {
#ifdef WIN32
				if (acl_last_error() != ERROR_FILE_NOT_FOUND)
#endif
				acl_msg_fatal("%s(%d): open file %s: %s(%d)", myname, __LINE__,
					db_path, acl_last_strerror(ebuf, sizeof(ebuf)), acl_last_error());
			}
		} else {
			if (acl_myflock(lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_SHARED) < 0)
				acl_msg_fatal("%s(%d): shared-lock database %s for open: %s",
					myname, __LINE__, path, acl_last_strerror(ebuf, sizeof(ebuf)));
		}
	}

	/*
	 * Use the DB 1.x programming interface. This is the default interface
	 * with 4.4BSD systems. It is also available via the db_185 compatibility
	 * interface, but that interface does not have the undocumented feature
	 * that we need to make file locking safe with POSIX fcntl() locking.
	 */
#if DB_VERSION_MAJOR < 2
	if ((db = dbopen(db_path, open_flags, 0644, type, tweak)) == 0)
		acl_msg_fatal("%s(%d): open database %s: %s",
			myname, __LINE__, db_path,
			acl_last_strerror(ebuf, sizeof(ebuf)));
	dbfd = db->fd(db);
#endif

	/*
	 * Use the DB 2.x programming interface. Jump a couple extra hoops.
	 */
#if DB_VERSION_MAJOR == 2
	db_flags = DB_FCNTL_LOCKING;
	if (open_flags == O_RDONLY)
		db_flags |= DB_RDONLY;
	if (open_flags & O_CREAT)
		db_flags |= DB_CREATE;
	if (open_flags & O_TRUNC)
		db_flags |= DB_TRUNCATE;
	if ((errno = db_open(db_path, type, db_flags, 0644, 0, tweak, &db)) != 0)
		acl_msg_fatal("%s(%d): open database %s: %s",
			myname, __LINE__, db_path,
			acl_last_strerror(ebuf, sizeof(ebuf)));
	if (db == 0)
		acl_msg_panic("db_open null result");
	if ((errno = db->fd(db, &dbfd)) != 0)
		acl_msg_fatal("%s(%d): get database file descriptor: %s",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
#endif

	/*
	 * Use the DB 3.x programming interface. Jump even more hoops.
	 */
#if DB_VERSION_MAJOR > 2
	if ((errno = db_env_create(&dbenv, 0)) != 0) {
		acl_msg_fatal("%s(%d): db_env_create error(%s)",
			myname, __LINE__, db_strerror(errno));
	}
	dbenv->set_errfile(dbenv, NULL);
	dbenv->set_errpfx(dbenv, NULL);
	dbenv->set_alloc(dbenv, dict_mem_malloc, dict_mem_realloc, dict_mem_free);
	if ((errno = dbenv->set_cachesize(dbenv, 0, dict_db_cache_size, 0)) != 0) {
		acl_msg_fatal("%s(%d): set DB_ENV cache size %d: %s",
			myname, __LINE__, dict_db_cache_size, db_strerror(errno));
	}
	if ((errno = dbenv->set_mp_mmapsize(dbenv, 1024 * 1024 * 256)) != 0) {
		acl_msg_fatal("%s(%d): set DB_ENV mmapsize size %d: %s",
			myname, __LINE__, 1024 * 1024 * 100, db_strerror(errno));
	}
#if 0
	if ((errno = dbenv->set_flags(dbenv, DB_CDB_ALLDB | DB_REGION_INIT, 1)) != 0) {
	 	acl_msg_fatal("set_flags error(%s)", db_strerror(errno));
	}
	if ((errno = dbenv->set_encrypt(dbenv, "zsxxsz", DB_ENCRYPT_AES)) != 0) {
		acl_msg_fatal("set_encrypt error(%s), errno(%d)", db_strerror(errno), errno);
	}
#endif
	env_flags = DB_CREATE
		/* | DB_INIT_LOCK */
		| DB_INIT_LOG
		| DB_PRIVATE
		| DB_INIT_MPOOL
		| DB_INIT_TXN
		| DB_THREAD
		/* | DB_USE_ENVIRON */;
	/*
	if ((errno = dbenv->set_data_dir(dbenv, db_home)) != 0) {
		acl_msg_fatal("%s(%d): set_data_dir(%s) error(%s)",
			myname, __LINE__, db_root, db_strerror(errno));
	}
	*/

	if ((errno = dbenv->open(dbenv, db_home, env_flags, 0)) != 0) {
		char  curpath[256];

		curpath[0] = 0;
		getcwd(curpath, sizeof(curpath));
		acl_msg_fatal("%s(%d): open db_home=%s, %s, current path=%s",
			myname, __LINE__, db_home, db_strerror(errno), curpath);
	} else {
		char  curpath[256];

		curpath[0] = 0;
		getcwd(curpath, sizeof(curpath));
		acl_msg_info("%s(%d): current path=%s", myname, __LINE__, curpath);
	}

	db_flags = /* DB_FCNTL_LOCKING |*/ DB_CREATE | DB_THREAD /* | DB_AUTO_COMMIT */;
	if (open_flags == O_RDONLY)
		db_flags |= DB_RDONLY;
	if (open_flags & O_CREAT)
		db_flags |= DB_CREATE;
	if (open_flags & O_TRUNC)
		db_flags |= DB_TRUNCATE;

	if ((errno = db_create(&db, dbenv, 0)) != 0)
		acl_msg_fatal("%s(%d): create DB database: %s, %s",
			myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)), db_strerror(errno));
	if (db == 0)
		acl_msg_panic("db_create null result");

	if (dict_db_page_size > 0) {
		if ((errno = db->set_pagesize(db, dict_db_page_size)) != 0) {
			acl_msg_warn("set pagesize error %s", db_strerror(errno));
		}
	}
	if ((errno = db->get_pagesize(db, (unsigned *) &size)) == 0) {
		acl_msg_info("open database: pagesize %d", size);
	}

	db->set_errfile(db, NULL);
	db->set_errpfx(db, NULL);
	if ((errno = db->set_bt_compare(db, dict_db_cmpkey_fn)) != 0) {
		acl_msg_fatal("%s(%d): set_bt_compare error(%s)",
			myname, __LINE__, db_strerror(errno));
	}

	if (type == DB_HASH && db->set_h_nelem(db, DICT_DB_NELM) != 0)
		acl_msg_fatal("%s(%d): set DB hash element count %d: %s",
			myname, __LINE__, DICT_DB_NELM,
			acl_last_strerror(ebuf, sizeof(ebuf)));
#if ((DB_VERSION_MAJOR == 4 || DB_VERSION_MAJOR == 5) && DB_VERSION_MINOR > 0)
	if ((errno = db->open(db, 0, db_name, 0, type, db_flags, 0644)) != 0) {
		char  curpath[256];

		curpath[0] = 0;
		(void) getcwd(curpath, sizeof(curpath));
		acl_msg_fatal("%s(%d): open database %s: %s, %s, current path=%s",
			myname, __LINE__, db_path,
			acl_last_strerror(ebuf, sizeof(ebuf)), db_strerror(errno),
			curpath);
	}
#elif (DB_VERSION_MAJOR == 3 || DB_VERSION_MAJOR == 4)
	if ((errno = db->open(db, db_name, 0, type, db_flags, 0644)) != 0)
		acl_msg_fatal("%s(%d): open database %s: %s",
			myname, __LINE__, db_path,
			acl_last_strerror(ebuf, sizeof(ebuf)));
#else
#error "Unsupported Berkeley DB version"
#endif
	if ((errno = db->fd(db, &dbfd)) != 0)
		acl_msg_fatal("%s(%d): get database file descriptor: %s",
			myname, __LINE__, acl_last_strerror(ebuf, sizeof(ebuf)));
#endif
	if ((dict_flags & DICT_FLAG_LOCK) && lock_fd != ACL_FILE_INVALID) {
		if (acl_myflock(lock_fd, ACL_FLOCK_STYLE_FLOCK, ACL_FLOCK_OP_NONE) < 0)
			acl_msg_fatal("%s(%d): unlock database %s for open: %s",
				myname, __LINE__, db_path,
				acl_last_strerror(ebuf, sizeof(ebuf)));
		if (acl_file_close(lock_fd) < 0)
			acl_msg_fatal("%s(%d): close database %s: %s",
				myname, __LINE__, db_path,
				acl_last_strerror(ebuf, sizeof(ebuf)));
	}
	dict_db = (DICT_DB *) dict_alloc(class, path, sizeof(*dict_db));
	dict_db->dict.lookup = dict_db_lookup;
	dict_db->dict.update = dict_db_update;
	dict_db->dict.delete_it = dict_db_delete;
	dict_db->dict.sequence = dict_db_sequence;
	dict_db->dict.sequence_reset = dict_db_sequence_reset;
	dict_db->dict.sequence_delcur = dict_db_sequence_delcur;
	dict_db->dict.close = dict_db_close;
	dict_db->dict.lock_fd = (ACL_FILE_HANDLE) FILE_HANDLE(dbfd);
	dict_db->dict.stat_fd = (ACL_FILE_HANDLE) FILE_HANDLE(dbfd);
#if DB_VERSION_MAJOR > 2
	dict_db->dbenv = dbenv;
	dict_db->quit  = 0;
#endif
	snprintf(dict_db->dict.db_path, sizeof(dict_db->dict.db_path), "%s", db_path);
	if (acl_stat(dict_db->dict.db_path, &st) < 0)
		acl_msg_fatal("%s(%d): dict_db_open: fstat(%s): %s",
			myname, __LINE__, db_path,
			acl_last_strerror(ebuf, sizeof(ebuf)));
	dict_db->dict.mtime = st.st_mtime;

	/*
	 * Warn if the source file is newer than the indexed file, except when
	 * the source file changed only seconds ago.
	 */
	if ((dict_flags & DICT_FLAG_LOCK) != 0
			&& acl_stat(path, &st) == 0
			&& st.st_mtime > dict_db->dict.mtime
			&& st.st_mtime < time((time_t *) 0) - 100)
		acl_msg_warn("database %s is older than source file %s", db_path, path);

#ifdef ACL_UNIX
	acl_close_on_exec(dict_db->dict.lock_fd, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(dict_db->dict.stat_fd, ACL_CLOSE_ON_EXEC);
#endif
	dict_db->dict.flags = dict_flags | DICT_FLAG_FIXED;
	if ((dict_flags & (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL)) == 0)
		dict_db->dict.flags |= (DICT_FLAG_TRY1NULL | DICT_FLAG_TRY0NULL);
	if (dict_flags & DICT_FLAG_FOLD_FIX)
		dict_db->dict.fold_buf = acl_vstring_alloc(10);
	dict_db->db = db;
#if DB_VERSION_MAJOR > 1
	dict_db->cursor = 0;
#endif

	acl_myfree(db_root);
	acl_myfree(db_home);
	acl_myfree(db_name);
	acl_myfree(db_path);
	acl_myfree(saved_path);

	if (dict_db)
		create_backend_threads(dict_db);

	return (DICT_DEBUG (&dict_db->dict));
}

/* dict_hash_open - create association with data base */

DICT   *dict_hash_open(const char *path, int open_flags, int dict_flags)
{
#if DB_VERSION_MAJOR < 2
	HASHINFO tweak;

	memset((char *) &tweak, 0, sizeof(tweak));
	tweak.nelem = DICT_DB_NELM;
	tweak.cachesize = dict_db_cache_size;
#endif
#if DB_VERSION_MAJOR == 2
	DB_INFO tweak;

	memset((char *) &tweak, 0, sizeof(tweak));
	tweak.h_nelem = DICT_DB_NELM;
	tweak.db_cachesize = dict_db_cache_size;
#endif
#if DB_VERSION_MAJOR > 2
	void   *tweak;

	tweak = 0;
#endif
	return (dict_db_open(DICT_TYPE_HASH, path, open_flags, DB_HASH,
		(void *) &tweak, dict_flags));
}

/* dict_btree_open - create association with data base */

DICT   *dict_btree_open(const char *path, int open_flags, int dict_flags)
{
#if DB_VERSION_MAJOR < 2
	BTREEINFO tweak;

	memset((char *) &tweak, 0, sizeof(tweak));
	tweak.cachesize = dict_db_cache_size;
#endif
#if DB_VERSION_MAJOR == 2
	DB_INFO tweak;

	memset((char *) &tweak, 0, sizeof(tweak));
	tweak.db_cachesize = dict_db_cache_size;
#endif
#if DB_VERSION_MAJOR > 2
	void   *tweak;

	tweak = 0;
#endif

	return (dict_db_open(DICT_TYPE_BTREE, path, open_flags, DB_BTREE,
		(void *) &tweak, dict_flags));
}

DB_ENV *dict_db_env(DICT *dict)
{
	DICT_DB *dict_db = (DICT_DB *) dict;

	return (dict_db->dbenv);
}

#endif  /* HAS_BDB */
