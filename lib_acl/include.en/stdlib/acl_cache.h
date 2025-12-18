#ifndef	ACL_CACHE_INCLUDE_H
#define	ACL_CACHE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif
#include "acl_define.h"
#include "acl_htable.h"
#include "acl_ring.h"
#include <time.h>

/**
 * Cache entry information stored in cache.
 */
typedef struct ACL_CACHE_INFO {
	char *key;		/**< Key value */
	void *value;		/**< User dynamic object */
	int   nrefer;		/**< Reference count */
	time_t when_timeout;	/**< Timeout time */
	ACL_RING entry;		/**< Internal ring member */
} ACL_CACHE_INFO;

/**
 * Cache object.
 */
typedef struct ACL_CACHE { 
	ACL_HTABLE *table;	/**< Hash table */
	ACL_RING ring;		/**< Ring for deletion order */
	int   max_size;		/**< Maximum cache size limit value */
	int   size;		/**< Current number of cache entries */
	int   timeout;		/**< Each cache entry's timeout time (seconds) */

	/**< Callback function to free user dynamic object */
	void  (*free_fn)(const ACL_CACHE_INFO*, void *);
	acl_pthread_mutex_t lock;	/**< Mutex lock */
	ACL_SLICE *slice;		/**< Memory slice pool */

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_CACHE*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_CACHE*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_CACHE*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_CACHE*);
	/* Get the current iterator's member structure object */
	ACL_CACHE_INFO *(*iter_info)(ACL_ITER*, struct ACL_CACHE*);
} ACL_CACHE;

/**
 * Create a cache object. When each cache entry times out,
 * will use user callback's space to free.
 * @param max_size {int} User cache's maximum entry count
 * @param timeout {int} Each cache entry's cache time
 * @param free_fn {void (*)(void*)} User callback function to free cache object
 * @return {ACL_CACHE*} Cache object pointer
 */
ACL_API ACL_CACHE *acl_cache_create(int max_size, int timeout,
	void (*free_fn)(const ACL_CACHE_INFO*, void*));

/**
 * Free a cache object, will automatically use free function
 * in acl_cache_create()/3 to free cache entries.
 * @param cache {ACL_CACHE*} Cache object pointer
 */
ACL_API void acl_cache_free(ACL_CACHE *cache);

/**
 * Add key-value object to cache object.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*} User-provided key value
 * @param value {void*} Dynamic object pointer
 * @return {ACL_CACHE_INFO*} Returned structure object's
 *  value is same as user's object, if return is NULL,
 *  indicates operation failed. Failure reason: cache is too
 *  full, or object with same value exists, or reference
 *  count is 0; if return is not NULL, indicates add
 *  succeeded. If same value is added repeatedly, new object
 *  replaces old data, and old data's free function is
 *  called to free
 */
ACL_API ACL_CACHE_INFO *acl_cache_enter(ACL_CACHE *cache, const char *key, void *value);

/**
 * Search for a certain key-value object from cache object.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*} Query key
 * @return {void*} Returned user data address, if NULL, indicates not found
 */
ACL_API void *acl_cache_find(ACL_CACHE *cache, const char *key);

/**
 * Search for a certain key-value object from cache object
 * and return cache entry information structure.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*} Query key
 * @return {ACL_CACHE_INFO*} Cache entry information
 *  structure address, if NULL, indicates not found
 */
ACL_API ACL_CACHE_INFO *acl_cache_locate(ACL_CACHE *cache, const char *key);

/**
 * Delete a certain cache entry from cache object.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param info {ACL_CACHE_INFO*} User-provided cache entry
 *  information structure
 * @return {int} 0: indicates delete succeeded; -1:
 *  indicates object's reference count is not 0, or object
 *  does not exist
 */
ACL_API int acl_cache_delete(ACL_CACHE *cache, ACL_CACHE_INFO *info);

/**
 * Delete a certain cache entry from cache object.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*} Key value
 * @return {int} 0: indicates delete succeeded; -1:
 *  indicates object's reference count is not 0, or object
 *  does not exist
 */
ACL_API int acl_cache_delete2(ACL_CACHE *cache, const char *key);

/**
 * Automatically delete timed-out entries in cache.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @return {int} >= 0: number of automatically deleted cache entries
 */
ACL_API int acl_cache_timeout(ACL_CACHE *cache);

/**
 * Extend a certain cache entry's cache timeout.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param info {ACL_CACHE_INFO*} Cache entry
 * @param timeout {int} Timeout time (seconds)
 */
ACL_API void acl_cache_update2(ACL_CACHE *cache, ACL_CACHE_INFO *info, int timeout);

/**
 * Extend a certain cache entry's cache timeout.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*} Key value
 * @param timeout {int} Timeout time (seconds)
 */
ACL_API void acl_cache_update(ACL_CACHE *cache, const char *key, int timeout);

/**
 * Increase a certain cache entry's reference count to
 * prevent it from being deleted currently.
 * @param info {ACL_CACHE_INFO*} User-provided cache entry
 *  information structure
 */
ACL_API void acl_cache_refer(ACL_CACHE_INFO *info);

/**
 * Increase a certain cache entry's reference count to
 * prevent it from being deleted currently.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*}
 */
ACL_API void acl_cache_refer2(ACL_CACHE *cache, const char *key);

/**
 * Decrease a certain cache entry's reference count.
 * @param info {ACL_CACHE_INFO*} User-provided cache entry
 *  information structure
 */
ACL_API void acl_cache_unrefer(ACL_CACHE_INFO *info);

/**
 * Decrease a certain cache entry's reference count.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param key {const char*}
 */
ACL_API void acl_cache_unrefer2(ACL_CACHE *cache, const char *key);

/**
 * Lock cache object when multi-threading.
 * @param cache {ACL_CACHE*} Cache object pointer
 */
ACL_API void acl_cache_lock(ACL_CACHE *cache);

/**
 * Unlock cache object when multi-threading.
 * @param cache {ACL_CACHE*} Cache object pointer
 */
ACL_API void acl_cache_unlock(ACL_CACHE *cache);

/**
 * Walk through all objects in cache.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param walk_fn {void (*)(ACL_CACHE_INFO*, void*)} Callback function
 * @param arg {void *} Second parameter in walk_fn()/2
 */
ACL_API void acl_cache_walk(ACL_CACHE *cache, void (*walk_fn)(ACL_CACHE_INFO *, void *), void *arg);

/**
 * Clean cache entries in cache object. If force is 0, only
 * entries with reference count 0 will be deleted; if force
 * is not 0, even entries with reference count not 0 will be
 * deleted.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @param force {int} If 0, only entries with reference count
 *  0 will be deleted; if not 0, even entries with reference
 *  count not 0 will be deleted
 * @return {int} Number of cleaned cache entries
 */
ACL_API int acl_cache_clean(ACL_CACHE *cache, int force);

/**
 * Current number of cache entries in cache object.
 * @param cache {ACL_CACHE*} Cache object pointer
 * @return {int} Number of cache entries
 */
ACL_API int acl_cache_size(ACL_CACHE *cache);

#ifdef	__cplusplus
}
#endif

#endif
