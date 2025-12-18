#ifndef ACL_HTABLE_INCLUDE_H
#define ACL_HTABLE_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "../thread/acl_thread.h"
#include "acl_hash.h"			/* just for ACL_HASH_FN */
#include "acl_slice.h"
#include "acl_iterator.h"

/*--------------------------------------------------------------------------*/
typedef struct ACL_HTABLE	ACL_HTABLE;
typedef struct ACL_HTABLE_INFO 	ACL_HTABLE_INFO;

/**
 * Hash table structure definition.
 */
struct ACL_HTABLE {
	int     size;                   /* length of entries array */
	int     init_size;              /* length of initial entryies array */
	int     used;                   /* number of entries in table */
	ACL_HTABLE_INFO **data;         /* entries array, auto-resized */
	unsigned int flag;              /* properties flag */
	int     status;                 /* the operator's status on the htable */

	ACL_HASH_FN  hash_fn;           /* hash function */
	ACL_SLICE_POOL *slice;
	acl_pthread_mutex_t *rwlock;

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_HTABLE*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_HTABLE*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_HTABLE*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_HTABLE*);
	/* Get the current iterator's member structure object */
	ACL_HTABLE_INFO *(*iter_info)(ACL_ITER*, struct ACL_HTABLE*);
};

/**
 * Storage information structure for each hash entry in hash table.
 */
struct ACL_HTABLE_INFO {
	/**
	 * Hash key, only needs to be freed when key is dynamically allocated.
	 * When hash table's flag bit is ACL_BINHASH_FLAG_KEY_REUSE,
	 * memory space needs to be freed manually.
	 */
	union {
		char *key;
		const char *c_key;
	} key;
	void   *value;			/**< associated value */
	unsigned hash;			/**< store the key's hash value */
	struct ACL_HTABLE_INFO *next;	/**< colliding entry */
	struct ACL_HTABLE_INFO *prev;	/**< colliding entry */
};

/**
 * ACL_HTABLE iterator structure.
 */
typedef struct ACL_HTABLE_ITER {
	/* public */
	ACL_HTABLE_INFO *ptr;

	/* private */
	int  i;
	int  size;
	ACL_HTABLE_INFO **h;
} ACL_HTABLE_ITER;

/**
 * Create hash table.
 * @param size Hash table size
 * @param flag {unsigned int} Hash table property flag bits, ACL_BINHASH_FLAG_xxx
 * @return Hash table head pointer, if NULL (indicates internal allocation error,
 *  need to check memory allocation)
 */
ACL_API ACL_HTABLE *acl_htable_create(int size, unsigned int flag);
/* Whether to directly use key address when creating new object */
#define	ACL_HTABLE_FLAG_KEY_REUSE	(1 << 0)

/* Whether to use multi-threaded mutual exclusion mode */
#define	ACL_HTABLE_FLAG_USE_LOCK	(1 << 1)

/* Whether to move queried entry to head each time query */
#define	ACL_HTABLE_FLAG_MSLOOK		(1 << 2)

/* Convert key to lowercase uniformly to achieve case-insensitive
 * query regardless of case */
#define	ACL_HTABLE_FLAG_KEY_LOWER	(1 << 3)

ACL_API ACL_HTABLE *acl_htable_create3(int size, unsigned int flag,
		ACL_SLICE_POOL *slice);

/**
 * Set hash table's control parameters.
 * @param table Hash table object
 * @param name Control parameter name initial value, name can be followed
 *  by control parameters:
 *  ACL_HTABLE_CTL_END: end parameter flag
 *  ACL_HTABLE_CTL_RWLOCK: whether to use read-write lock
 *  ACL_HTABLE_CTL_HASH_FN: user-defined hash value calculation function
 */
ACL_API void acl_htable_ctl(ACL_HTABLE *table, int name, ...);
#define	ACL_HTABLE_CTL_END      0  /**< Control end flag */
#define	ACL_HTABLE_CTL_RWLOCK   1  /**< Whether to lock */
#define	ACL_HTABLE_CTL_HASH_FN  2  /**< Set private hash function */

/**
 * Get hash table's status after last hash table operation.
 * @param table Hash table pointer
 * @return Hash table operation status, see ACL_HTABLE_STAT_XXX below
 */
ACL_API int acl_htable_errno(ACL_HTABLE *table);
#define	ACL_HTABLE_STAT_OK          0  /**< Status OK */
#define	ACL_HTABLE_STAT_INVAL       1  /**< Invalid parameter */
#define	ACL_HTABLE_STAT_DUPLEX_KEY  2  /**< Duplicate key */

/**
 * Set hash table's current status, error value ACL_HTABLE_STAT_XXX
 * @param table Hash table pointer
 * @param error Error status to set for hash table
 */
ACL_API void acl_htable_set_errno(ACL_HTABLE *table, int error);

ACL_API void acl_htable_lock_read(ACL_HTABLE *table);
ACL_API void acl_htable_lock_write(ACL_HTABLE *table);
ACL_API void acl_htable_unlock(ACL_HTABLE *table);

/**
 * Add new entry to hash table.
 * @param table Hash table pointer
 * @param key Key, internally copies key string
 * @param value User's own object pointer (must be dynamically allocated,
 *  cannot be stack variable or local variable)
 * @return Created hash entry pointer, == NULL: indicates internal memory
 *  allocation error Note: If hash entry already exists when adding, returns
 *  existing hash entry. Application should check via acl_htable_errno()
 *  to see if duplicate key exists (ACL_HTABLE_STAT_DUPLEX_KEY)
 */
ACL_API ACL_HTABLE_INFO *acl_htable_enter(ACL_HTABLE *table,
		const char *key, void *value);

/**
 * Add new entry to hash table, when multiple threads simultaneously perform
 * this operation, internally automatically ensures thread safety.
 * @param table Hash table pointer
 * @param key Key, internally copies key string
 * @param value User's own object pointer (must be dynamically allocated,
 *  cannot be stack variable or local variable)
 * @param old_holder {void**} If key exists and this is not NULL, internally
 *  automatically replaces value and stores old value address in old_holder
 *  for application to free
 * @return Created hash entry pointer, == NULL: indicates internal memory
 *  allocation error.
 *  Note: If hash entry already exists when adding, returns existing hash entry.
 *  Application should check via acl_htable_errno() to see if duplicate key
 *  exists (ACL_HTABLE_STAT_DUPLEX_KEY)
 */
ACL_API ACL_HTABLE_INFO *acl_htable_enter_r2(ACL_HTABLE *table,
		const char *key, void *value, void **old_holder);

ACL_API ACL_HTABLE_INFO *acl_htable_enter_r(ACL_HTABLE *table,
		const char *key, void *value);

/**
 * Search for a specific hash entry based on key.
 * @param table Hash table pointer
 * @param key Key
 * @return Non-NULL pointer: indicates found hash entry corresponding to key
 *         NULL: indicates hash entry corresponding to key not found
 */
ACL_API ACL_HTABLE_INFO *acl_htable_locate(ACL_HTABLE *table, const char *key);

/**
 * Search for a specific hash entry based on key, when multiple threads
 * simultaneously perform this operation,
 * internally automatically ensures thread safety.
 * @param table Hash table pointer
 * @param key Key
 * @return Non-NULL pointer: indicates found hash entry corresponding to key
 *         NULL: indicates hash entry corresponding to key not found
 */
ACL_API ACL_HTABLE_INFO *acl_htable_locate_r(ACL_HTABLE *table, const char *key);

/**
 * Search for user data value based on key.
 * @param table Hash table pointer
 * @param key Key
 * @return Non-NULL: indicates found data value corresponding to key, user
 *  can cast to user's own data type; NULL: indicates data value corresponding
 *  to key not found
 */
ACL_API void *acl_htable_find(ACL_HTABLE *table, const char *key);

/**
 * Search for user data value based on key, when multiple threads simultaneously
 *  perform this operation,internally automatically ensures thread safety.
 * @param table Hash table pointer
 * @param key Key
 * @return Non-NULL: indicates found data value corresponding to key, user can
 *  cast to user's own data type; NULL: indicates data value corresponding to
 *  key not found
 */
ACL_API void *acl_htable_find_r(ACL_HTABLE *table, const char *key);

/**
 * Delete a hash entry based on key.
 * @param table Hash table pointer
 * @param key Key
 * @param free_fn If this function pointer is not NULL and found data value
 *  corresponding to key, internally calls user-provided function to perform
 *  some cleanup, then frees the hash entry
 * @return 0: success;  -1: key not found
 */
ACL_API int acl_htable_delete(ACL_HTABLE *table, const char *key,
		void (*free_fn) (void *));
#define	acl_htable_delete_r	acl_htable_delete

/**
 * Directly delete entry from hash table based on non-NULL object returned
 * by acl_htable_locate.
 * @param table Hash table pointer
 * @param ht {ACL_HTABLE_INFO*} Internal structure object stored in hash table
 * @param free_fn If this function pointer is not NULL and found data value
 *  corresponding to key, internally calls user-provided function to perform
 *  some cleanup, then frees the hash entry
 */
ACL_API void acl_htable_delete_entry(ACL_HTABLE *table, ACL_HTABLE_INFO *ht,
		void (*free_fn) (void *));

/**
 * Free entire hash table.
 * @param table Hash table pointer
 * @param free_fn If pointer is not NULL, for each hash entry in hash table,
 *  calls this function for cleanup, then frees
 */
ACL_API void acl_htable_free(ACL_HTABLE *table, void (*free_fn) (void *));

/**
 * Reset hash table, this function frees all data in hash table, then reinitializes.
 * @param table Hash table pointer
 * @param free_fn If pointer is not NULL, for each hash entry in hash table,
 *  calls this function for cleanup, then frees
 * @return Whether reset succeeded. 0: OK; < 0: error.
 */
ACL_API int acl_htable_reset(ACL_HTABLE *table, void (*free_fn) (void *));
#define	acl_htable_reset_r	acl_htable_reset

/**
 * Walk through each hash entry in hash table.
 * @param table Hash table pointer
 * @param walk_fn Function pointer for each hash entry, must not be NULL
 * @param arg User's own passed parameter
 */
ACL_API void acl_htable_walk(ACL_HTABLE *table,
		void (*walk_fn) (ACL_HTABLE_INFO *, void *), void *arg);
#define	acl_htable_walk_r	acl_htable_walk

/**
 * Get hash table's current allocated space size.
 * @param table Hash table pointer
 * @return Hash table's allocated space size
 */
ACL_API int acl_htable_size(const ACL_HTABLE *table);

/**
 * Get hash table's current number of stored entry elements.
 * @param table Hash table pointer
 * @return Hash table element count
 */
ACL_API int acl_htable_used(const ACL_HTABLE *table);

/**
 * Convert hash table's entries into an array.
 * @param table Hash table
 * @return Non-NULL: array pointer; NULL: indicates hash table has no hash entries
 */
ACL_API ACL_HTABLE_INFO **acl_htable_list(const ACL_HTABLE *table);

/**
 * Display hash table's key distribution status.
 * @param table Hash table pointer
 */
ACL_API void acl_htable_stat(const ACL_HTABLE *table);
#define	acl_htable_stat_r	acl_htable_stat

ACL_API ACL_HTABLE_INFO **acl_htable_data(ACL_HTABLE *table);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_head(
		ACL_HTABLE *table, ACL_HTABLE_ITER *iter);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_next(ACL_HTABLE_ITER *iter);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_tail(
		ACL_HTABLE *table, ACL_HTABLE_ITER *iter);
ACL_API const ACL_HTABLE_INFO *acl_htable_iter_prev(ACL_HTABLE_ITER *iter);

/*--------------------  Some helper macros --------------------------------*/

#define	ACL_HTABLE_ITER_KEY(iter)	((iter).ptr->key.c_key)
#define	acl_htable_iter_key		ACL_HTABLE_ITER_KEY

#define	ACL_HTABLE_ITER_VALUE(iter)	((iter).ptr->value)
#define	acl_htable_iter_value		ACL_HTABLE_ITER_VALUE

/**
 * Iterate ACL_HTABLE
 * @param iter {ACL_HTABLE_ITER}
 * @param table_ptr {ACL_HTABLE *}
 * @example:
	void test()
	{
		ACL_HTABLE *table = acl_htable_create(10, 0);
		ACL_HTABLE_ITER iter;
		char *value, key[32];
		int   i;

		for (i = 0; i < 100; i++) {
			value = (char*) acl_mystrdup("value");
			snprintf(key, sizeof(key), "key:%d", i);
			(void) acl_htable_enter(table, key, value);
		}

		acl_htable_foreach(iter, table) {
			printf("%s=%s\n", acl_htable_iter_key(iter),
				acl_htable_iter_value(iter));
			if (i == 50)
				break;
		}
		acl_htable_free(table, acl_myfree_fn);
	}
 */

#if 0
#define	ACL_HTABLE_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_htable_size((table_ptr)), (iter).i = 0,  \
          (iter).h = acl_htable_data((table_ptr)); (iter).i < (iter).size; (iter).i++)  \
            for ((iter).ptr = *(iter).h++; (iter).ptr; (iter).ptr = (iter).ptr->next)
#define	ACL_HTABLE_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_htable_size((table_ptr)), (iter).i = (iter).size - 1,  \
          (iter).h = acl_htable_data((table_ptr)) + (iter).i; (iter).i >= 0; (iter).i--)  \
            for ((iter).ptr = *(iter).h--; (iter).ptr; (iter).ptr = (iter).ptr->next)
#else
#define	ACL_HTABLE_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_htable_iter_head((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_htable_iter_next(&iter))
#define	ACL_HTABLE_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_htable_iter_tail((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_htable_iter_prev(&iter))
#endif

#define	acl_htable_foreach		ACL_HTABLE_FOREACH
#define	acl_htable_foreach_reverse	ACL_HTABLE_FOREACH_REVERSE

#ifdef  __cplusplus
}
#endif

#endif
