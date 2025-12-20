#ifndef	ACL_DBUF_POOL_INCLUDE_H
#define	ACL_DBUF_POOL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

typedef struct ACL_DBUF_POOL ACL_DBUF_POOL;

/**
 * Create memory pool object.
 * @param block_size {size_t} Memory pool's size for each allocated memory block, in bytes.
 * @return {ACL_DBUF_POOL*} Returns NULL on error
 */
ACL_API ACL_DBUF_POOL *acl_dbuf_pool_create(size_t block_size);

/**
 * Create memory pool object.
 * @param block_size {size_t} Memory pool's size for each allocated memory block, in bytes.
 * @param align {size_t} Byte alignment when allocating memory
 * @return {ACL_DBUF_POOL*} Returns NULL on error
 */
ACL_API ACL_DBUF_POOL *acl_dbuf_pool_create2(size_t block_size, size_t align);

/**
 * Reset memory pool state, will free all allocated memory data blocks.
 * @param pool {ACL_DBUF_POOL*} Memory pool object
 * @param off {size_t} Offset position of memory block to reset to
 * @return {int} Return 0 indicates reset succeeded, non-zero indicates failure
 */
ACL_API int  acl_dbuf_pool_reset(ACL_DBUF_POOL *pool, size_t off);

/**
 * Destroy memory pool.
 * @param pool {ACL_DBUF_POOL*} Pool object
 */
ACL_API void acl_dbuf_pool_destroy(ACL_DBUF_POOL *pool);

/**
 * Allocate memory of specified length.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param  len {size_t} Required memory size
 * @return {void*} Newly allocated memory address
 */
ACL_API void *acl_dbuf_pool_alloc(ACL_DBUF_POOL *pool, size_t len);

/**
 * Allocate memory of specified length and initialize memory to zero.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param len {size_t} Required memory length
 * @return {void*} Newly allocated memory address
 */
ACL_API void *acl_dbuf_pool_calloc(ACL_DBUF_POOL *pool, size_t len);

/**
 * Dynamically allocate new memory for source string and copy string content, similar to strdup.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param s {const char*} Source string
 * @return {char*} Address of newly copied string
 */
ACL_API char *acl_dbuf_pool_strdup(ACL_DBUF_POOL *pool, const char *s);

/**
 * Dynamically allocate new memory for source string and copy string content, similar to strdup.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param s {const char*} Source string
 * @param len {size_t} Maximum string length to copy
 * @return {char*} Address of newly copied string
 */
ACL_API char *acl_dbuf_pool_strndup(ACL_DBUF_POOL *pool,
	const char *s, size_t len);

/**
 * Dynamically allocate memory for source memory data and copy data content.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param addr {const void*} Source memory data address
 * @param len {size_t} Source data length
 * @return {void*} Address of newly copied data
 */
ACL_API void *acl_dbuf_pool_memdup(ACL_DBUF_POOL *pool,
		const void *addr, size_t len);

/**
 * Return memory to memory pool for reuse.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param addr {const void*} Memory address allocated from memory pool
 * @return {int} If memory address is not in memory pool or freed multiple times, returns -1; if operation succeeded,
 *  returns 0
 */
ACL_API int acl_dbuf_pool_free(ACL_DBUF_POOL *pool, const void *addr);

/**
 * Keep a certain address in memory pool, so it will not be freed when calling reset.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param addr {const void*} Memory address allocated from memory pool
 * @return {int} If operation succeeded, returns 0; if memory address is not in memory pool, returns -1
 */
ACL_API int acl_dbuf_pool_keep(ACL_DBUF_POOL *pool, const void *addr);

/**
 * Remove a certain address from memory pool's kept addresses, so it will be freed when calling dbuf_reset.
 * @param pool {ACL_DBUF_POOL*} Pool object
 * @param addr {const void*} Memory address allocated from memory pool
 * @return {int} If operation succeeded, returns 0; if memory address is not in memory pool, returns -1
 */
ACL_API int acl_dbuf_pool_unkeep(ACL_DBUF_POOL *pool, const void *addr);

/**
 * Internal test function.
 */
ACL_API void acl_dbuf_pool_test(size_t max);

#ifdef	__cplusplus
}
#endif

#endif
