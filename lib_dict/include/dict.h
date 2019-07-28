#ifndef _DICT_H_INCLUDED_
#define _DICT_H_INCLUDED_

#include "lib_acl.h"
#include <fcntl.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef DICT_DLL
# ifdef DICT_EXPORTS
#  define DICT_API __declspec(dllexport)
# else
#  define DICT_API __declspec(dllimport)
# endif
#else
#  define DICT_API
#endif

 /*
  * Generic dictionary interface - in reality, a dictionary extends this
  * structure with private members to maintain internal state.
  */
typedef struct DICT {
	char   *type;                 /* for diagnostics */
	char   *name;                 /* for diagnostics */
	int     flags;                /* see below */
	const char *(*lookup) (struct DICT *, char *, size_t, char **, size_t *);
	void    (*update) (struct DICT *, char *, size_t, char *, size_t);
	int     (*delete_it) (struct DICT *, char *, size_t);
	int     (*sequence) (struct DICT *, int, char **, size_t *, char **, size_t *);
	void    (*sequence_reset)(struct DICT *);
	int     (*sequence_delcur)(struct DICT *);
	void    (*close) (struct DICT *);
	ACL_FILE_HANDLE lock_fd;      /* for dict_update() lock */
	ACL_FILE_HANDLE stat_fd;      /* change detection */
	char    db_path[256];
	time_t  mtime;                /* mod time at open */
	ACL_VSTRING *fold_buf;        /* key folding buffer */
} DICT;

DICT_API DICT *dict_alloc(const char *dict_type, const char *dict_name, size_t size);
DICT_API void dict_free(DICT *dict);

DICT_API DICT *dict_debug_main(DICT *dict);

#define DICT_DEBUG(d) ((d)->flags & DICT_FLAG_DEBUG ? dict_debug_main(d) : (d))

#define DICT_FLAG_NONE		(0)
#define DICT_FLAG_DUP_WARN	(1<<0)	/* if file, warn about dups */
#define DICT_FLAG_DUP_IGNORE	(1<<1)	/* if file, ignore dups */
#define DICT_FLAG_TRY0NULL	(1<<2)	/* do not append 0 to key/value */
#define DICT_FLAG_TRY1NULL	(1<<3)	/* append 0 to key/value */
#define DICT_FLAG_FIXED		(1<<4)	/* fixed key map */
#define DICT_FLAG_PATTERN	(1<<5)	/* keys are patterns */
#define DICT_FLAG_LOCK		(1<<6)	/* lock before access */
#define DICT_FLAG_DUP_REPLACE	(1<<7)	/* if file, replace dups */
#define DICT_FLAG_SYNC_UPDATE	(1<<8)	/* if file, sync updates */
#define DICT_FLAG_DEBUG		(1<<9)	/* log access */
#define DICT_FLAG_NO_REGSUB	(1<<11)	/* disallow regexp substitution */
#define DICT_FLAG_NO_PROXY	(1<<12)	/* disallow proxy mapping */
#define DICT_FLAG_NO_UNAUTH	(1<<13)	/* disallow unauthenticated data */
#define DICT_FLAG_FOLD_FIX	(1<<14)	/* case-fold key with fixed-case map */
#define DICT_FLAG_FOLD_MUL	(1<<15)	/* case-fold key with multi-case map */
#define DICT_FLAG_FOLD_ANY	(DICT_FLAG_FOLD_FIX | DICT_FLAG_FOLD_MUL)

 /* IMPORTANT: Update the dict_mask[] table when the above changes */

 /*
  * The subsets of flags that control how a map is used. These are relevant
  * mainly for proxymap support. Note: some categories overlap.
  * 
  * DICT_FLAG_PARANOID - flags that forbid the use of insecure map types for
  * security-sensitive operations. These flags are specified by the caller,
  * and are checked by the map implementation itself upon open, lookup etc.
  * requests.
  * 
  * DICT_FLAG_IMPL_MASK - flags that specify properties of the lookup table
  * implementation. These flags are set by the map implementation itself.
  * 
  * DICT_FLAG_INST_MASK - flags that control how a specific table instance is
  * opened or used. The caller specifies these flags, and the caller may not
  * change them between open, lookup, etc. requests (although the map itself
  * may make changes to some of these flags).
  * 
  * DICT_FLAG_NP_INST_MASK - ditto, but without the paranoia flags.
  * 
  * DICT_FLAG_RQST_MASK - flags that the caller specifies, and that the caller
  * may change between open, lookup etc. requests.
  */
#define DICT_FLAG_PARANOID \
	(DICT_FLAG_NO_REGSUB | DICT_FLAG_NO_PROXY | DICT_FLAG_NO_UNAUTH)
#define DICT_FLAG_IMPL_MASK	(DICT_FLAG_FIXED | DICT_FLAG_PATTERN)
#define DICT_FLAG_RQST_MASK	(DICT_FLAG_FOLD_ANY | DICT_FLAG_LOCK | \
				DICT_FLAG_DUP_REPLACE | DICT_FLAG_DUP_WARN | \
				DICT_FLAG_SYNC_UPDATE)
#define DICT_FLAG_NP_INST_MASK	~(DICT_FLAG_IMPL_MASK | DICT_FLAG_RQST_MASK)
#define DICT_FLAG_INST_MASK	(DICT_FLAG_NP_INST_MASK | DICT_FLAG_PARANOID)

/* global variables */
extern int dict_unknown_allowed;
extern __thread int dict_errno;

/* bdb global variables */
extern int dict_db_cache_size;
extern int dict_db_page_size;

#ifdef HAS_DB
#include "db.h"
extern int (*dict_db_cmpkey_fn)(DB*, const DBT*, const DBT*);
#endif

#define DICT_ERR_NONE	0		/* no error */
#define DICT_ERR_RETRY	1		/* soft error */

 /*
  * Sequence function types.
  */
#define DICT_SEQ_FUN_FIRST     0	/* set cursor to first record */
#define DICT_SEQ_FUN_NEXT      1	/* set cursor to next record */

 /*
  * Interface for dictionary types.
  */
DICT_API ACL_ARGV *dict_mapnames(void);

 /*
  * High-level interface, with logical dictionary names.
  */
DICT_API void dict_init(void);
DICT_API void dict_register(const char *dict_name, DICT *dict_info);
DICT_API DICT *dict_handle(const char *dict_name);
DICT_API void dict_unregister(const char *dict_name);
DICT_API void dict_update(const char *dict_name, char *key, char *value, size_t len);
DICT_API const char *dict_lookup(const char *dict_name, char *member,
	char **value, size_t *size);
DICT_API int dict_delete(const char *dict_name, char *key);
DICT_API int dict_sequence(const char *dict_name, const int func, char **key,
	size_t *key_size, char **value, size_t *value_size);
DICT_API void dict_sequence_reset(const char *dict_name);
DICT_API int dict_sequence_delcur(const char *dict_name);
DICT_API void dict_load_file(const char *dict_name, const char *path);
DICT_API void dict_load_fp(const char *dict_name, ACL_VSTREAM *fp);
DICT_API const char *dict_eval(char *dict_name, const char *value, int recursive);

 /*
  * Low-level interface, with physical dictionary handles.
  */
DICT_API void dict_open_init(void);
DICT_API DICT *dict_open(const char *dict_spec, int open_flags, int dict_flags);
DICT_API DICT *dict_open3(const char *dict_type, const char *dict_name,
	int open_flags, int dict_flags);
DICT_API void dict_open_register(const char *type, DICT *(*open_fn) (const char *, int, int));

#define DICT_GET(dp, key, key_len, value, size) \
	((const char *) (dp)->lookup((dp), (key), (key_len), (value), (size)))
#define DICT_PUT(dp, key, key_len, val, len) \
	(dp)->update((dp), (key), (key_len), (val), (len))
#define DICT_DEL(dp, key, key_len) \
	(dp)->delete_it((dp), (key), (key_len))
#define DICT_SEQ(dp, f, key, key_size, val, val_size) \
	(dp)->sequence((dp), (f), (key), (key_size), (val), (val_size))
#define	DICT_RESET(dp) \
	(dp->sequence_reset(dp))
#define	DICT_DELCUR(dp) \
	(dp->sequence_delcur(dp))
#define DICT_CLOSE(dp) \
	(dp)->close(dp)

typedef void (*DICT_WALK_ACTION) (const char *, DICT *, char *);
DICT_API void dict_walk(DICT_WALK_ACTION action, char *ptr);
DICT_API int dict_changed(void);
DICT_API const char *dict_changed_name(void);
DICT_API const char *dict_flags_str(int dict_flags);

#ifdef __cplusplus
}
#endif

#endif
