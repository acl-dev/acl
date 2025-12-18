#ifndef ACL_BINHASH_INCLUDE_H
#define ACL_BINHASH_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_hash.h"			/* just for ACL_HASH_FN */
#include "acl_slice.h"
#include "acl_iterator.h"

typedef struct ACL_BINHASH ACL_BINHASH;
typedef struct ACL_BINHASH_INFO ACL_BINHASH_INFO;

/**     
 * Structure of one hash table.
 */
struct ACL_BINHASH {
	int     size;                   /**< length of entries array */
	int     used;                   /**< number of entries in table */
	unsigned int flag;              /**< the hash table's properties flag */
	int     status;                 /**< the hash tables' operation status */
	ACL_BINHASH_INFO **data;        /**< entries array, auto-resized */
	ACL_SLICE *slice;               /**< memory slice */
	ACL_HASH_FN hash_fn;            /**< hash function */

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_BINHASH*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_BINHASH*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_BINHASH*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_BINHASH*);
	/* Get the current iterator's member structure object */
	ACL_BINHASH_INFO *(*iter_info)(ACL_ITER*, struct ACL_BINHASH*);
};

/**
 * Structure of one hash table entry.
 */
struct ACL_BINHASH_INFO {
	union {
		void *key;
		const void *c_key;
	} key;				/**
					 * Hash key, only needs to
					 * be freed when key is
					 * dynamically allocated. When
					 * hash table's flag bit is
					 * ACL_BINHASH_FLAG_KEY_REUSE, memory space needs to be freed manually.
					 */
	int     key_len;                /**< Hash key length */
	void   *value;                  /**< Hash key's corresponding user data */
	struct ACL_BINHASH_INFO *next;  /**< colliding entry */
	struct ACL_BINHASH_INFO *prev;  /**< colliding entry */
};

/**
 * ACL_BINHASH iterator structure.
 */
typedef struct ACL_BINHASH_ITER {
	/* public */
	ACL_BINHASH_INFO *ptr;

	/* private */
	int  i;
	int  size;
	ACL_BINHASH_INFO **h;
} ACL_BINHASH_ITER;

/**
 * Create a hash table.
 * @param size {int} Hash table's initial array size
 * @param flag {unsigned int} Hash table's property flag
 *  bits, ACL_BINHASH_FLAG_xxx
 * @return {ACL_BINHASH*} Newly created hash table pointer
 */
ACL_API ACL_BINHASH *acl_binhash_create(int size, unsigned int flag);
#define	ACL_BINHASH_FLAG_KEY_REUSE	(1 << 0)
#define	ACL_BINHASH_FLAG_SLICE_RTGC_OFF	(1 << 1)
#define	ACL_BINHASH_FLAG_SLICE1		(1 << 2)
#define	ACL_BINHASH_FLAG_SLICE2		(1 << 3)
#define	ACL_BINHASH_FLAG_SLICE3		(1 << 4)

/**
 * Add object to hash table.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @param key {const void*} Hash key
 * @param key_len {int} key's length
 * @param value {void*} Value
 * @return {ACL_BINHASH_INFO*} Newly created hash entry pointer
 */
ACL_API ACL_BINHASH_INFO *acl_binhash_enter(ACL_BINHASH *table, const void *key, int key_len, void *value);

/**
 * Get corresponding hash entry from hash table based on key.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @param key {const void*} Hash key
 * @param key_len {int} key's length
 * @return {ACL_BINHASH_INFO*} Hash entry pointer
 */
ACL_API ACL_BINHASH_INFO *acl_binhash_locate(ACL_BINHASH *table, const void *key, int key_len);

/**
 * Query a certain hash table's key value.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @param key {const void*} Hash key
 * @param key_len {int} key's length
 * @return {void*} Hash key value
 */
ACL_API void *acl_binhash_find(ACL_BINHASH *table, const void *key, int key_len);

/**
 * Delete a certain hash entry.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @param key {const void*} Hash key
 * @param key_len {int} key's length
 * @param free_fn {void (*)(void*)} Callback function pointer
 *  to free hash key value, if NULL, internally frees key
 *  value
 * @return {int} 0: ok, -1: error
 */
ACL_API int acl_binhash_delete(ACL_BINHASH *table, const void *key, int key_len, void (*free_fn) (void *));

/**
 * Free hash table.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @param free_fn {void (*)(void*)} If not NULL, use this
 *  function to free all key values in hash table
 */
ACL_API void acl_binhash_free(ACL_BINHASH *table, void (*free_fn) (void *));

/**
 * Walk through all entries in hash table, call
 * user-provided callback function to process each key value
 * in hash table.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @param walk_fn {void (*)(ACL_BINHASH_INFO*, void*)}
 *  Callback function when walking through each element in
 *  hash table
 * @param arg {void*} User-provided parameter, passed to walk_fn
 */
ACL_API void acl_binhash_walk(ACL_BINHASH *table, void (*walk_fn) (ACL_BINHASH_INFO *, void *), void *arg);

/**
 * List all elements in current hash table as a list.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @return {ACL_BINHASH_INFO*} Array of ACL_BINHASH_INFO
 *  composed of hash table's all elements, 
 *  the last pointer in array is NULL
 */
ACL_API ACL_BINHASH_INFO **acl_binhash_list(ACL_BINHASH *table);

/**
 * Get error number when hash table operation fails.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @return {int} Error number
 */
ACL_API int acl_binhash_errno(ACL_BINHASH *table);
#define ACL_BINHASH_STAT_OK		0
#define ACL_BINHASH_STAT_INVAL		1
#define ACL_BINHASH_STAT_DUPLEX_KEY	2
#define	ACL_BINHASH_STAT_NO_KEY		3

/**
 * Get hash table's current allocated space size.
 * @param table Hash table pointer
 * @return Hash table's allocated space size
 */
ACL_API int acl_binhash_size(const ACL_BINHASH *table);

/**
 * Current number of objects in hash table.
 * @param table {ACL_BINHASH*} Hash table pointer
 * @return {int}
 */
ACL_API int acl_binhash_used(ACL_BINHASH *table);

ACL_API ACL_BINHASH_INFO **acl_binhash_data(ACL_BINHASH *table);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_head(ACL_BINHASH *table, ACL_BINHASH_ITER *iter);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_next(ACL_BINHASH_ITER *iter);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_tail(ACL_BINHASH *table, ACL_BINHASH_ITER *iter);
ACL_API const ACL_BINHASH_INFO *acl_binhash_iter_prev(ACL_BINHASH_ITER *iter);

/*--------------------  Some helper macros --------------------------------*/

#define	ACL_BINHASH_ITER_KEY(iter)	((iter).ptr->key.c_key)
#define	acl_binhash_iter_key		ACL_BINHASH_ITER_KEY

#define	ACL_BINHASH_ITER_VALUE(iter)	((iter).ptr->value)
#define	acl_binhash_iter_value		ACL_BINHASH_ITER_VALUE

/**
 * Iterate ACL_BINHASH
 * @param iter {ACL_BINHASH_ITER}
 * @param table_ptr {ACL_BINHASH *}
 * @example:
	void test()
	{
		ACL_BINHASH *table = acl_binhash_create(10, 0);
		ACL_BINHASH_ITER iter;
		char *value, key[32];
		int   i;

		for (i = 0; i < 100; i++) {
			value = (char*) acl_mystrdup("value");
			snprintf(key, sizeof(key), "key:%d", i);
			(void) acl_binhash_enter(table, key, strlen(key), value);
		}

		acl_binhash_foreach(iter, table) {
			printf("%s=%s\n", iter.ptr->key.c_key, iter.ptr->value);
			if (i == 50)
				break;
		}

		acl_binhash_free(table, acl_myfree_fn);
	}
 */
#if 0
#define	ACL_BINHASH_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_binhash_size((table_ptr)), (iter).i = 0,  \
          (iter).h = acl_binhash_data((table_ptr)); (iter).i < (iter).size; (iter).i++)  \
            for ((iter).ptr = *(iter).h++; (iter).ptr; (iter).ptr = (iter).ptr->next)
#define	ACL_BINHASH_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((iter).size = acl_binhash_size((table_ptr)), (iter).i = (iter).size - 1,  \
          (iter).h = acl_binhash_data((table_ptr)) + (iter).i; (iter).i >= 0; (iter).i--)  \
            for ((iter).ptr = *(iter).h--; (iter).ptr; (iter).ptr = (iter).ptr->next)
#else
#define	ACL_BINHASH_FOREACH(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_binhash_iter_head((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_binhash_iter_next(&iter))
#define	ACL_BINHASH_FOREACH_REVERSE(iter, table_ptr)  \
    if (table_ptr)  \
        for((void) acl_binhash_iter_tail((table_ptr), &iter);  \
            (iter).ptr;  \
            (void) acl_binhash_iter_prev(&iter))
#endif

#define	acl_binhash_foreach		ACL_BINHASH_FOREACH
#define	acl_binhash_foreach_reverse	ACL_BINHASH_FOREACH_REVERSE

#ifdef  __cplusplus
}
#endif

#endif
