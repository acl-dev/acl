#ifndef	ACL_CACHE2_INCLUDE_H
#define	ACL_CACHE2_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif
#include "acl_define.h"
#include <time.h>
#include "acl_iterator.h"

typedef struct ACL_CACHE2 ACL_CACHE2;

/**
 * Cache entry structure for key-value storage.
 */
typedef struct ACL_CACHE2_INFO {
	char  *key;		/**< Key value */
	void  *value;		/**< User dynamic data */
	int    nrefer;		/**< Reference count */
	time_t when_timeout;	/**< Expiration timestamp */
	ACL_CACHE2* cache;	/**< Associated cache object */
} ACL_CACHE2_INFO;

/**
 * Cache object.
 */
struct ACL_CACHE2 { 
	int   max_size;		/**< Maximum cache size limit value */
	int   size;		/**< Current number of cache entries */
	void *ctx;		/**< External context object */

	/**< Callback function to free user dynamic data */
	void  (*free_fn)(const ACL_CACHE2_INFO*, void *);

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_CACHE2*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_CACHE2*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_CACHE2*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_CACHE2*);
	/* Get the current iterator's member structure object */
	ACL_CACHE2_INFO *(*iter_info)(ACL_ITER*, struct ACL_CACHE2*);
};

/**
 * static void free_callback(const ACL_CACHE2_INFO *info, void *value) {
 * 	printf("Free key=%s\r\n", info->key);
 * 	free(value);
 * }
 *
 * void test() {
 * 	ACL_CACHE2 *cache = acl_cache2_create(100, free_callback);
 * 	const char *key = "test-key";
 * 	char *buf = strdup("test-value"), *value;
 * 	acl_cache2_enter(cache, key, value, 100);
 * 	value = (char*) acl_cache2_find(cache, key);
 * 	printf("The key=%s, value=%s\r\n", key, value ? value : "Not found");
 * 	ACL_ITER iter;
 * 	acl_foreach(iter, cache) {
 * 		value = (char*) iter.data;
 * 		key = iter.key;
 * 		...
 * 	}
 * }
 */

/**
 * Create a cache object. When each cache entry expires, the user callback
 * will be invoked to free the associated memory.
 * @param max_size {int} User-defined cache size limit; if value <= 0,
 *  internal size limit will be used instead
 * @param free_fn {void (*)(void*)} User-defined callback function to free cache entries
 * @return {ACL_CACHE2*} Cache object pointer
 */
ACL_API ACL_CACHE2 *acl_cache2_create(int max_size,
	void (*free_fn)(const ACL_CACHE2_INFO*, void*));

/**
 * Free a cache object.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 */
ACL_API void acl_cache2_free(ACL_CACHE2 *cache2);

/**
 * Add a key-value object to the cache.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*} Key value to be stored
 * @param value {void*} Dynamic data object
 * @param timeout {int} Cache timeout for each entry (seconds)
 * @return {ACL_CACHE2_INFO*} Returns the structure object containing the entry;
 *  the value field is the same as the user-provided value. Returns NULL on failure;
 *  failure reasons: cache is full, or an object with the same key exists and
 *  its reference count is 0. Returns non-NULL on success; if the same key
 *  is added again, the new data replaces the old data, and the old data's
 *  free callback is invoked to free it.
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_enter(ACL_CACHE2 *cache2,
	const char *key, void *value, int timeout);

/**
 * Find an object in the cache by key.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*} Query key
 * @return {void*} Returns the user-provided address; NULL if not found
 */
ACL_API void *acl_cache2_find(ACL_CACHE2 *cache2, const char *key);

/**
 * Find an object in the cache by key and return its cache info structure.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*} Query key
 * @return {ACL_CACHE2_INFO*} Cache info structure address; NULL if not found
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_locate(ACL_CACHE2 *cache2, const char *key);

/**
 * Delete a cache entry from the cache.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param info {ACL_CACHE2_INFO*} Cache info structure returned by user
 * @return {int} 0: deletion successful; -1: object reference count is not 0, or object does not exist
 */
ACL_API int acl_cache2_delete(ACL_CACHE2 *cache2, ACL_CACHE2_INFO *info);

/**
 * Delete a cache entry from the cache by key.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*} Key value
 * @return {int} 0: deletion successful; -1: object reference count is not 0,
 *  or object does not exist
 */
ACL_API int acl_cache2_delete2(ACL_CACHE2 *cache2, const char *key);

/**
 * Automatically delete all expired objects in the cache.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @return {int} >= 0: number of automatically deleted cache entries
 */
ACL_API int acl_cache2_timeout(ACL_CACHE2 *cache2);

/**
 * Extend the cache timeout for a cache entry.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param info {ACL_CACHE2_INFO*} Cache entry
 * @param timeout {int} New timeout (seconds)
 * @return {ACL_CACHE2_INFO*} Returns non-NULL on success; NULL if key does not exist
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_update2(ACL_CACHE2 *cache2,
	ACL_CACHE2_INFO *info, int timeout);

/**
 * Extend the cache timeout for a cache entry.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*} Key value
 * @param timeout {int} New timeout (seconds)
 * @return {ACL_CACHE2_INFO*} Returns non-NULL on success; NULL if key does not exist
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_update(ACL_CACHE2 *cache2,
	const char *key, int timeout);

/**
 * Insert or update an object in the cache.
 * @param key {const char*} Key value to be stored
 * @param value {void*} Dynamic data object
 * @param timeout {int} Cache timeout for each entry (seconds)
 * @param exist {int*} If non-NULL, indicates whether key existed: 1 if existed, 0 if new
 * @return {ACL_CACHE2_INFO*} Returns the structure object containing the entry;
 *  the value field is the same as the user-provided value. If the key existed,
 *  the new object replaces the old object; returns NULL on failure.
 *  If this is an update operation, the caller should free the old dynamic data.
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_upsert(ACL_CACHE2 *cache2,
	const char *key, void *value, int timeout, int *exist);
	
/**
 * Get the head entry of the timeout queue; callers can access the corresponding
 * object via ACL_CACHE2_INFO::value.
 * @param cache2 {ACL_CACHE2*}
 * @return {ACL_CACHE2_INFO*} Returns NULL if cache is empty
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_head(ACL_CACHE2 *cache2);

/**
 * Get the tail entry of the timeout queue; callers can access the corresponding
 * object via ACL_CACHE2_INFO::value.
 * @param cache2 {ACL_CACHE2*}
 * @return {ACL_CACHE2_INFO*} Returns NULL if cache is empty
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_tail(ACL_CACHE2 *cache2);

/**
 * Increment the reference count for a cache entry to prevent it from being deleted.
 * @param info {ACL_CACHE2_INFO*} Cache info structure returned by user
 */
ACL_API void acl_cache2_refer(ACL_CACHE2_INFO *info);

/**
 * Increment the reference count for a cache entry to prevent it from being deleted.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*}
 */
ACL_API void acl_cache2_refer2(ACL_CACHE2 *cache2, const char *key);

/**
 * Decrement the reference count for a cache entry.
 * @param info {ACL_CACHE2_INFO*} Cache info structure returned by user
 */
ACL_API void acl_cache2_unrefer(ACL_CACHE2_INFO *info);

/**
 * Decrement the reference count for a cache entry.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param key {const char*}
 */
ACL_API void acl_cache2_unrefer2(ACL_CACHE2 *cache2, const char *key);

/**
 * Lock the cache object for multi-threaded access.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 */
ACL_API void acl_cache2_lock(ACL_CACHE2 *cache2);

/**
 * Unlock the cache object for multi-threaded access.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 */
ACL_API void acl_cache2_unlock(ACL_CACHE2 *cache2);

/**
 * Walk through all objects in the cache.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param walk_fn {void (*)(ACL_CACHE2_INFO*, void*)} User callback function
 * @param arg {void *} Second argument to walk_fn()
 */
ACL_API void acl_cache2_walk(ACL_CACHE2 *cache2,
	void (*walk_fn)(ACL_CACHE2_INFO *, void *), void *arg);

/**
 * Clean cache entries in the timeout queue that exceed a certain reference count;
 * if force is set, entries with reference count 0 will also be deleted.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @param force {int} If non-zero, entries with reference count 0 will also be deleted
 * @return {int} Number of cleaned cache entries
 */
ACL_API int acl_cache2_clean(ACL_CACHE2 *cache2, int force);

/**
 * Get the current number of cache entries.
 * @param cache2 {ACL_CACHE2*} Cache object pointer
 * @return {int} Number of cached objects
 */
ACL_API int acl_cache2_size(ACL_CACHE2 *cache2);

#ifdef	__cplusplus
}
#endif

#endif
