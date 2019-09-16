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

ACL_API void acl_memory_debug_start(void);
ACL_API void acl_memory_debug_stop(void);
ACL_API void acl_memory_debug_stack(int onoff);
ACL_API void acl_memory_stat(void);
ACL_API void acl_memory_alloc_stat(void);

/**
 * 打开动态内存池功能
 * @param max_size {size_t} 内存池最大空间大小，单位为字节
 * @param use_mutex {int} 内存池内部是否采用互斥锁，如果是多线程程序则应该
 *  设置 use_mutex 为非0值
 */
ACL_API void acl_mempool_open(size_t max_size, int use_mutex);

/**
 * 关闭内存池功能
 */
ACL_API void acl_mempool_close(void);

/**
 * 当内存池打开后，可以通过此函数控制内存池状态
 */
ACL_API void acl_mempool_ctl(int name, ...);
#define	ACL_MEMPOOL_CTL_END         0  /**< 结束标志 */
#define	ACL_MEMPOOL_CTL_MUTEX       1  /**< 控制内存池是否加锁 */
#define	ACL_MEMPOOL_CTL_DISABLE     2  /**< 是否关闭内存池 */

/**
 * 当前内存池已经分配的内存大小
 * @return {int} 已经分配的内存大小
 */
ACL_API int acl_mempool_total_allocated(void);

/*---------------- ACL库中缺省的内存分配、释放等管理接口 -------------------*/

/**
 * 获得当前内存指针的一些状态信息，如该内存的实际大小与对外分配大小
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param ptr {void*} 动态分配的内存外部地址
 * @param len {size_t*} 存储该内存的外部可用大小
 * @param real_len {size*} 存储该内存的实际大小(因为内部有一些控制字节)
 */
ACL_API void acl_default_memstat(const char *filename, int line,
        void *ptr, size_t *len, size_t *real_len);

ACL_API void acl_default_meminfo(void);

/**
 * 设置内存分配最大报警值，即当调用者分配的内存大小达到此报警值后，内部会自动
 * 记录报警日志，同时将调用堆栈打印至日志中；内部缺少值是 100000000
 * @param len {size_t} 最大报警值，该值必须 > 0
 */
ACL_API void acl_default_set_memlimit(size_t len);

/**
 * 获得当前所设置的内存分配最大报警值大小(内部缺省值是 100000000)
 * @return {size_t}
 */
ACL_API size_t acl_default_get_memlimit(void);

/**
 * ACL库中缺省的内存分配器接口, 分配内存但并不初始化所分配内存的内容
 * 类似于标准库中的 malloc
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param size {size_t} 需要的内存大小
 * @return {void*} 分配的可用地址, 如果分配失败，则内部会自动coredump
 *   需要调用 acl_default_free 释放
 */
ACL_API void *acl_default_malloc(const char *filename, int line, size_t size);

/**
 * ACL库中缺省的内存分配器接口, 分配内存并初始化所分配内存的内容为0
 * 类似于标准库中的 calloc
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param nmemb {size_t} 内存块的个数
 * @param size {size_t} 每个内存块的大小
 * @return {void*} 分配的可用地址, 如果分配失败，则内部会自动coredump
 *   需要调用 acl_default_free 释放
 */
ACL_API void *acl_default_calloc(const char *filename, int line,
		size_t nmemb, size_t size);

/**
 * ACL库中缺省的内存分配器接口, 类似于标准库的 realloc
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param ptr {void*} 之前用ACL库所分配的内存地址
 * @param size {size_t} 需要的内存大小
 * @return {void*} 分配的可用地址, 如果分配失败，则内部会自动coredump
 *   需要调用 acl_default_free 释放
 */
ACL_API void *acl_default_realloc(const char *filename, int line,
		void *ptr, size_t size);

/**
 * 复制字符串，类似于标准库中的 strdup
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param str {const char*} 源字符串地址
 * @return {char*} 新复制的字符串地址，需要调用 acl_default_free 释放
 */
ACL_API char *acl_default_strdup(const char *filename, int line, const char *str);

/**
 * 复制字符串，但限制最大字符串长度，类似于标准库中的 strndup
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param str {const char*} 源字符串地址
 * @param len {size_t} 限制新字符串的最大长度值
 * @return {char*} 新复制的字符串地址，需要调用 acl_default_free 释放
 */
ACL_API char *acl_default_strndup(const char *filename, int line,
		const char *str, size_t len);

/**
 * 复制内存数据
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 * @param ptr {const void*} 源内存地址
 * @param len {size_t} 源内存区域的长度
 * @return {void*} 新复制的内存地址 
 */
ACL_API void *acl_default_memdup(const char *filename, int line,
		const void *ptr, size_t len);

/**
 * 释放由 acl_devault_xxx 所分配的内存动态内存
 * @param filename {const char*} 调用该函数的文件名，可以为空
 * @param line {int} 调用该函数所在源文件中的行数
 */
ACL_API void  acl_default_free(const char *filename, int line, void *ptr);

/*----- acl_mymalloc.h 内存管理接口中的宏调用所使用的内存管理函数接口 ------*/

/* 该函数接口集其实是调用了其它的内存管理来进行内存的分配与释放等管理操作的，
 * 它提供了高级宏调用的外部使用接口，方便用户操作。
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
