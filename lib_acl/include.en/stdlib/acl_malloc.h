#ifndef ACL_MALLOC_INCLUDE_H
#define ACL_MALLOC_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/*
 * Memory alignment of memory allocator results.
 * By default we align for doubles.
 */

#ifndef ALIGN_TYPE
# if defined(__hpux) && defined(__ia64)
#  define ALIGN_TYPE	__float80
# elif defined(__ia64__)
#  define ALIGN_TYPE	long double
# else
#  define ALIGN_TYPE	size_t
# endif
#endif

ACL_API int *acl_memory_debug_start(void);
ACL_API void acl_memory_debug_stop(void);
ACL_API void acl_memory_debug_stack(int onoff);
ACL_API void acl_memory_stat(void);
ACL_API void acl_memory_alloc_stat(void);

/**
 * Open dynamic memory pool manager.
 * @param max_size {size_t} Memory pool's maximum space size, unit is bytes
 * @param use_mutex {int} Whether memory pool internally uses
 *  mutex lock. For multi-threaded applications, should set
 *  use_mutex to non-zero value
 */
ACL_API void acl_mempool_open(size_t max_size, int use_mutex);

/**
 * Close memory pool manager.
 */
ACL_API void acl_mempool_close(void);

/**
 * After memory pool is opened, can control memory pool
 * state through this function.
 */
ACL_API void acl_mempool_ctl(int name, ...);
#define	ACL_MEMPOOL_CTL_END         0  /**< End flag */
#define	ACL_MEMPOOL_CTL_MUTEX       1  /**< Set whether memory pool uses
					*   lock */
#define	ACL_MEMPOOL_CTL_DISABLE     2  /**< Whether to disable memory pool */

/**
 * Current memory pool's total allocated memory size.
 * @return {int} Total allocated memory size
 */
ACL_API int acl_mempool_total_allocated(void);

/**
 * Print current memory pool status information to log or screen.
 */
ACL_API void acl_mempool_status(void);

/*---------------- ACL library's default memory allocation,
 * deallocation and other management interfaces -------------------*/

/**
 * Get some status information about current memory pointer,
 * including memory's actual size and allocated size.
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param ptr {void*} Dynamically allocated memory's external address
 * @param len {size_t*} Storage for memory's external allocated size
 * @param real_len {size*} Storage for memory's actual size
 *  (because internally has some overhead bytes)
 */
ACL_API void acl_default_memstat(const char *filename, int line,
        void *ptr, size_t *len, size_t *real_len);

ACL_API void acl_default_meminfo(void);

/**
 * Set memory allocation's maximum alarm value. When application's allocated
 * memory size reaches this alarm value, internally automatically records
 * error log, and simultaneously uses stack trace to print in log.
 * Internal default value is 100000000
 * @param len {size_t} Maximum alarm value, value must be > 0
 */
ACL_API void acl_default_set_memlimit(size_t len);

/**
 * Get current application's memory allocation's maximum
 * alarm value size (internal default value is 100000000).
 * @return {size_t}
 */
ACL_API size_t acl_default_get_memlimit(void);

/**
 * ACL library's default memory allocation interface,
 * allocates memory but does not initialize. Memory allocation
 * is similar to standard library's malloc
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param size {size_t} Required memory size
 * @return {void*} Allocated memory address, if allocation
 *  fails, internally automatically coredump. Need to use
 *  acl_default_free to free
 */
ACL_API void *acl_default_malloc(const char *filename, int line, size_t size);

/**
 * ACL library's default memory allocation interface,
 * allocates memory and initializes memory content to 0.
 * Similar to standard library's calloc
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param nmemb {size_t} Number of memory blocks
 * @param size {size_t} Size of each memory block
 * @return {void*} Allocated memory address, if allocation
 *  fails, internally automatically coredump. Need to use
 *  acl_default_free to free
 */
ACL_API void *acl_default_calloc(const char *filename, int line,
		size_t nmemb, size_t size);

/**
 * ACL library's default memory reallocation interface,
 * similar to standard library's realloc
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param ptr {void*} Previously ACL-allocated memory address
 * @param size {size_t} Required memory size
 * @return {void*} Allocated memory address, if allocation
 *  fails, internally automatically coredump. Need to use
 *  acl_default_free to free
 */
ACL_API void *acl_default_realloc(const char *filename, int line,
		void *ptr, size_t size);

/**
 * Copy string, similar to standard library's strdup
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param str {const char*} Source string address
 * @return {char*} Address of newly copied string, need to
 *  use acl_default_free to free
 */
ACL_API char *acl_default_strdup(const char *filename, int line, const char *str);

/**
 * Copy string, with maximum string length limit, similar to
 * standard library's strndup
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param str {const char*} Source string address
 * @param len {size_t} Maximum string length value to copy
 * @return {char*} Address of newly copied string, need to
 *  use acl_default_free to free
 */
ACL_API char *acl_default_strndup(const char *filename, int line,
		const char *str, size_t len);

/**
 * Copy memory data.
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 * @param ptr {const void*} Source memory address
 * @param len {size_t} Source memory data length
 * @return {void*} Address of newly copied memory 
 */
ACL_API void *acl_default_memdup(const char *filename, int line,
		const void *ptr, size_t len);

/**
 * Free memory dynamically allocated via acl_devault_xxx functions.
 * @param filename {const char*} File name where this
 *  function is called, can be NULL
 * @param line {int} Line number in source file where this
 *  function is called
 */
ACL_API void  acl_default_free(const char *filename, int line, void *ptr);

/*----- Memory allocation interfaces used by functions in
 * acl_mymalloc.h memory management interface ------*/

/* This function interface is actually a wrapper for ACL library's
 * internal memory allocation and deallocation management functions,
 * providing high-level convenient external use interface, users can
 * customize
 */

ACL_API void *acl_malloc_glue(const char *filename, int line, size_t size);
ACL_API void *acl_calloc_glue(const char *filename, int line, size_t nmemb, size_t size);
ACL_API void *acl_realloc_glue(const char *filename, int line, void *ptr, size_t size);
ACL_API char *acl_strdup_glue(const char *filename, int line, const char *str);
ACL_API char *acl_strndup_glue(const char *filename, int line, const char *str, size_t len);
ACL_API void *acl_memdup_glue(const char *filename, int line, const void *ptr, size_t len);
ACL_API void  acl_free_glue(const char *filename, int line, void *ptr);
ACL_API void  acl_free_fn_glue(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
