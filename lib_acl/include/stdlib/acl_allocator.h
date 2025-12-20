#ifndef ACL_ALLOCATOR_INCLUDE_H
#define ACL_ALLOCATOR_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef	ACL_PREPARE_COMPILE
#include "acl_define.h"
#include <stdlib.h>
#endif

typedef enum {
	ACL_MEM_TYPE_NONE,
	ACL_MEM_TYPE_8_BUF,
	ACL_MEM_TYPE_16_BUF,
	ACL_MEM_TYPE_32_BUF,
	ACL_MEM_TYPE_64_BUF,
	ACL_MEM_TYPE_128_BUF,
	ACL_MEM_TYPE_256_BUF,
	ACL_MEM_TYPE_512_BUF,
	ACL_MEM_TYPE_1K_BUF,
	ACL_MEM_TYPE_2K_BUF,
	ACL_MEM_TYPE_4K_BUF,
	ACL_MEM_TYPE_8K_BUF,
	ACL_MEM_TYPE_16K_BUF,
	ACL_MEM_TYPE_32K_BUF,
	ACL_MEM_TYPE_64K_BUF,
	ACL_MEM_TYPE_128K_BUF,
	ACL_MEM_TYPE_256K_BUF,
	ACL_MEM_TYPE_512K_BUF,
	ACL_MEM_TYPE_1M_BUF,
	ACL_MEM_TYPE_VSTRING,
	ACL_MEM_TYPE_MAX
} acl_mem_type;

typedef struct ACL_MEM_POOL ACL_MEM_POOL;
typedef struct ACL_ALLOCATOR ACL_ALLOCATOR;

/* in acl_mpool.c */
/**
 * Create a memory allocator object.
 * @param mem_limit {size_t} Memory pool's maximum memory, unit is bytes
 * @return {ACL_ALLOCATOR *} Memory allocator object pointer
 */
ACL_API ACL_ALLOCATOR *acl_allocator_create(size_t mem_limit);

/**
 * Set some parameters for memory allocator.
 * @param name {int} First parameter in parameter list
 * Usage:
 * acl_allocator_ctl(ACL_ALLOCATOR_CTL_MIN_SIZE, 128,
 *		ACL_ALLOCATOR_CTL_MAX_SIZE, 1024,
 *		ACL_ALLOCATOR_CTL_END);
 */
ACL_API void acl_allocator_ctl(int name, ...);

#define ACL_ALLOCATOR_CTL_END		0    /**< Control end flag */
#define ACL_ALLOCATOR_CTL_MIN_SIZE	1    /**< Minimum byte size */
#define ACL_ALLOCATOR_CTL_MAX_SIZE	2    /**< Maximum byte size */

/**
 * Set memory allocator's maximum size.
 * @param allocator {ACL_ALLOCATOR*}
 * @param mem_limit {size_t} Memory pool's maximum value, unit is bytes
 */
ACL_API void acl_allocator_config(ACL_ALLOCATOR *allocator, size_t mem_limit);

/**
 * Free memory allocator object and all allocated memory.
 * @param allocator {ACL_ALLOCATOR*}
 */
ACL_API void acl_allocator_free(ACL_ALLOCATOR *allocator);

/**
 * Add a new memory pool to memory allocator.
 * @param allocator {ACL_ALLOCATOR*}
 * @param label {const char*} Descriptive information for this memory pool type
 * @param obj_size {size_t} Size of each object in this
 *  memory pool type, unit is bytes
 * @param type {acl_mem_type} Memory type
 * @param after_alloc_fn {void (*)(void*, void*)} Callback
 *  function called after memory allocation succeeds, can be
 *  NULL
 * @param before_free_fn {void (*)(void*, void*)} Callback
 *  function called before freeing memory, can be NULL
 * @param pool_ctx {void*} Application's own private context
 *  object, when after_alloc_fn and before_free_fn are not
 *  NULL, this parameter will be directly passed to
 *  application
 * @return {ACL_MEM_POOL*} Memory pool object corresponding
 *  to this memory pool type
 */
ACL_API ACL_MEM_POOL *acl_allocator_pool_add(ACL_ALLOCATOR *allocator,
					const char *label,
					size_t obj_size,
					acl_mem_type type,
					void (*after_alloc_fn)(void *obj, void *pool_ctx),
					void (*before_free_fn)(void *obj, void *pool_ctx),
					void *pool_ctx);

/**
 * Remove a memory pool from memory allocator.
 * @param allocator {ACL_ALLOCATOR*}
 * @param pool {ACL_MEM_POOL*} Object returned by acl_allocatore_pool_add
 */
ACL_API void acl_allocator_pool_remove(ACL_ALLOCATOR *allocator, ACL_MEM_POOL *pool);

/**
 * Probe whether a certain memory type has corresponding
 * memory pool in memory allocator.
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} Memory type
 * @return {int}, 0: no; != 0: yes
 */
ACL_API int acl_allocator_pool_ifused(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * Number of memory objects currently in use for a certain memory type.
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} Memory type
 * @return {int} Number of memory objects currently in use
 *  for a certain memory pool type
 */
ACL_API int acl_allocator_pool_inuse_count(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * Total size of memory currently in use for a certain
 * memory type in memory pool.
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} Memory type
 * @return {int} Total size of memory currently in use for
 *  a certain memory type in memory pool, unit is bytes
 */
ACL_API int acl_allocator_pool_inuse_size(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * Total size of memory currently in use in memory allocator.
 * @param allocator {ACL_ALLOCATOR*}
 * @return {int} Memory size, unit is bytes
 */
ACL_API int acl_allocator_pool_total_allocated(ACL_ALLOCATOR *allocator);

/**
 * Allocate memory for a certain memory type.
 * @param filename {const char*}
 * @param line {int}
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} Memory type
 * @return {void*} Newly allocated memory address
 */
ACL_API void *acl_allocator_mem_alloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * Free memory space for a certain memory type.
 * @param filename {const char*}
 * @param line {int}
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} Memory type
 * @param obj {void*} Memory object to free, must not be NULL
 */
ACL_API void acl_allocator_mem_free(const char *filename, int line,
	ACL_ALLOCATOR *allocator, acl_mem_type type, void *obj);

/**
 * Automatically match memory pool type based on required
 * memory size, find matching type, and allocate memory.
 * If no memory pool matches, directly call acl_mymalloc to allocate memory.
 * @param filename {const char*} Current file name where macro is defined
 * @param line {int} Current file line number where macro is defined
 * @param allocator {ACL_ALLOCATOR*}
 * @param size {size_t} Required memory size to allocate
 * @return {void*} Newly allocated memory address
 */
ACL_API void *acl_allocator_membuf_alloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, size_t size);

/**
 * Reallocate memory space based on required memory size,
 * find matching type, and allocate memory.
 * If no memory pool matches, directly call acl_mymalloc to allocate memory.
 * @param filename {const char*} Current file name where macro is defined
 * @param line {int} Current file line number where macro is defined
 * @param allocator {ACL_ALLOCATOR*}
 * @param oldbuf {void*} Originally allocated memory
 * @param size {size_t} Required memory size
 * @return {void*} Newly allocated memory address
 */
ACL_API void *acl_allocator_membuf_realloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, void *oldbuf, size_t size);

/**
 * Free memory, automatically find matching size memory pool
 * memory type, and free it. Otherwise, directly call
 * acl_myfree to free.
 * @param filename {const char*} Current file name where macro is defined
 * @param line {int} Current file line number where macro is defined
 * @param allocator {ACL_ALLOCATOR*}
 * @param buf {void*} Memory address
 */
ACL_API void acl_allocator_membuf_free(const char *filename, int line,
	ACL_ALLOCATOR *allocator, void *buf);

#ifdef __cplusplus
}
#endif

#endif
