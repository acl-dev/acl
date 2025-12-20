#ifndef ACL_MEM_HOOK_INCLUDE_H
#define ACL_MEM_HOOK_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Register hook functions for memory allocation and deallocation. When ACL internally
 * allocates or deallocates memory, it will call the registered functions.
 * When registering these functions, you must ensure that these function pointers are valid.
 * @param malloc_hook {void *(*)(const char* fname, int lineno, size_t)}
 * @param calloc_hook {void *(*)(const char* fname, int lineno, size_t, size_t)}
 * @param realloc_hook {void *(*)(const char* fname, int lineno, void *, size_t)}
 * @param strdup_hook {void *(*)(const char* fname, int lineno, const char*)}
 * @param strndup_hook {void *(*)(const char* fname, int lineno, const char*, size_t)}
 * @param memdup_hook {void *(*)(const char* fname, int lineno, const void *, size_t)}
 * @param free_hook {void (*)(const char* fname, int lineno, void*)}
 */
ACL_API void acl_mem_hook(void *(*malloc_hook)(const char*, int, size_t),
		void *(*calloc_hook)(const char*, int, size_t, size_t),
		void *(*realloc_hook)(const char*, int, void*, size_t),
		char *(*strdup_hook)(const char*, int, const char*),
		char *(*strndup_hook)(const char*, int, const char*, size_t),
		void *(*memdup_hook)(const char*, int, const void*, size_t),
		void  (*free_hook)(const char*, int, void*));

/**
 * Remove previously set memory hook functions and restore to default state.
 */
ACL_API void acl_mem_unhook(void);

#ifdef __cplusplus
}
#endif

#endif
