#ifndef ACL_MEM_HOOK_INCLUDE_H
#define ACL_MEM_HOOK_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 设置内存分配、释放的注册函数，当ACL内部分配释放内存时便调用这些注册的函数
 * 在调用此函数进行注册时必须保证这几个函数指针参数均非空
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
 * 取消之前设置的内存勾子函数，恢复为缺省状态
 */
ACL_API void acl_mem_unhook(void);

#ifdef __cplusplus
}
#endif

#endif
