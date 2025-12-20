#ifndef	ACL_SLICE_INCLUDE_H
#define	ACL_SLICE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

#define	ACL_SLICE_FLAG_OFF		(0)
#define	ACL_SLICE_FLAG_GC1		(1 << 0)  /**< Space saving, gc
						 *  performance poor */
#define	ACL_SLICE_FLAG_GC2		(1 << 1)  /**< Space moderate, gc
						 *  relatively good */
#define	ACL_SLICE_FLAG_GC3		(1 << 2)  /**< Space wasteful, gc
						 *  only runs when idle */
#define	ACL_SLICE_FLAG_RTGC_OFF		(1 << 10) /**< Disable real-time
						 *  memory release */
#define	ACL_SLICE_FLAG_LP64_ALIGN	(1 << 11) /**< Whether to align to
						 *  8 bytes for 64-bit
						 *  platform requirements */

/**
 * Memory slice pool's status structure.
 */
typedef struct ACL_SLICE_STAT {
	int   nslots;           /**< total slice count free in slots */
	int   islots;           /**< current position of free slots slice */
	int   page_nslots;      /**< count slice of each page */
	int   page_size;        /**< length of each malloc */
	int   slice_length;	/**< length of each slice from user's set */
	int   slice_size;       /**< length of each slice really allocated */
	int   nbuf;             /**< count of MEM_BUF allocated */
	acl_uint64 length;      /**< total size of all MEM_BUF's buf */
	acl_uint64 used_length; /**< total size of used */
	unsigned int flag;	/**< same as the ACL_SLICE's flag been set when created */
} ACL_SLICE_STAT;

typedef struct ACL_SLICE ACL_SLICE;

/**
 * Create memory slice pool object.
 * @param name {const char*} Identifier name, for debugging
 * @param page_size {int} Page size when allocating memory
 * @param slice_size {int} Size of each fixed-length memory slice
 * @param flag {unsigned int} Flag bits, see ACL_SLICE_FLAG_xxx below
 * @return {ACL_SLICE*} Memory slice pool object pointer
 */
ACL_API ACL_SLICE *acl_slice_create(const char *name, int page_size,
	int slice_size, unsigned int flag);

/**
 * Destroy a memory slice pool object.
 * @param slice {ACL_SLICE*} Memory slice pool object
 */
ACL_API void acl_slice_destroy(ACL_SLICE *slice);

/**
 * Check how many memory slices in memory slice pool are currently in use.
 * @param slice {ACL_SLICE*} Memory slice pool object
 * @return {int} >= 0, number of memory slices currently in use
 */
ACL_API int acl_slice_used(ACL_SLICE *slice);

/**
 * Allocate a memory slice.
 * @param slice {ACL_SLICE*} Memory slice pool object
 * @return {void*} Memory slice address
 */
ACL_API void *acl_slice_alloc(ACL_SLICE *slice);

/**
 * Allocate a memory slice and initialize memory slice data to zero.
 * @param slice {ACL_SLICE*} Memory slice pool object
 * @return {void*} Memory slice address
 */
ACL_API void *acl_slice_calloc(ACL_SLICE *slice);

/**
 * Free a memory slice.
 * @param slice {ACL_SLICE*} Memory slice pool object
 * @param ptr {void*} Memory slice address, must be returned by
 *  acl_slice_alloc/acl_slice_calloc function
 */
ACL_API void acl_slice_free2(ACL_SLICE *slice, void *ptr);

/**
 * Free a memory slice.
 * @param ptr {void*} Memory slice address, must be returned by
 *  acl_slice_alloc/acl_slice_calloc function
 */
ACL_API void acl_slice_free(void *ptr);

/**
 * Check memory slice pool's current status.
 * @param slice {ACL_SLICE*} Memory slice pool object
 * @param sbuf {ACL_SLICE_STAT*} Storage buffer, must not be NULL
 */
ACL_API void acl_slice_stat(ACL_SLICE *slice, ACL_SLICE_STAT *sbuf);

/**
 * Manually trigger memory slice pool's unused memory to be released.
 * @param slice {ACL_SLICE*} Memory slice pool object
 */
ACL_API int acl_slice_gc(ACL_SLICE *slice);

/*------------------------*/

typedef struct ACL_SLICE_POOL {
	ACL_SLICE **slices;		/* the slice array */
	int   base;			/* the base byte size */
	int   nslice;			/* the max number of base size */
	unsigned int slice_flag;	/* flag: ACL_SLICE_FLAG_GC2[3] |
					 * ACL_SLICE_FLAG_RTGC_OFF */
} ACL_SLICE_POOL;

ACL_API void acl_slice_pool_init(ACL_SLICE_POOL *asp);
ACL_API ACL_SLICE_POOL *acl_slice_pool_create(int base, int nslice,
	unsigned int slice_flag);
ACL_API void acl_slice_pool_destroy(ACL_SLICE_POOL *asp);
ACL_API int acl_slice_pool_used(ACL_SLICE_POOL *asp);
ACL_API void acl_slice_pool_clean(ACL_SLICE_POOL *asp);
ACL_API void acl_slice_pool_reset(ACL_SLICE_POOL *asp);
ACL_API void acl_slice_pool_free(const char *filename, int line, void *buf);
ACL_API void acl_slice_pool_gc(ACL_SLICE_POOL *asp);
ACL_API void *acl_slice_pool_alloc(const char *filename, int line,
	ACL_SLICE_POOL *asp, size_t size);
ACL_API void *acl_slice_pool_calloc(const char *filename, int line,
	ACL_SLICE_POOL *asp, size_t nmemb, size_t size);
ACL_API void *acl_slice_pool_realloc(const char *filename, int line,
	ACL_SLICE_POOL *asp, void *ptr, size_t size);
ACL_API void *acl_slice_pool_memdup(const char *filename, int line,
	ACL_SLICE_POOL *asp, const void *ptr, size_t len);
ACL_API char *acl_slice_pool_strdup(const char *filename, int line,
	ACL_SLICE_POOL *asp, const char *str);
ACL_API char *acl_slice_pool_strndup(const char *filename, int line,
	ACL_SLICE_POOL *asp, const char *str, size_t len);

ACL_API void acl_slice_mem_hook(void *(*malloc_hook)(const char*, int, size_t),
		void *(*calloc_hook)(const char*, int, size_t, size_t),
		void *(*realloc_hook)(const char*, int, void*, size_t),
		void  (*free_hook)(const char*, int, void*));
ACL_API void acl_slice_mem_unhook(void);

#ifdef	__cplusplus
}
#endif

#endif
