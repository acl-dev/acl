#ifndef	ACL_SLICE_INCLUDE_H
#define	ACL_SLICE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"

#define	ACL_SLICE_FLAG_OFF		(0)
#define	ACL_SLICE_FLAG_GC1		(1 << 0)  /**< 空间节省, 但 gc 性能差 */
#define	ACL_SLICE_FLAG_GC2		(1 << 1)  /**< 空间中等, gc 比较好 */
#define	ACL_SLICE_FLAG_GC3		(1 << 2)  /**< 空间最大, gc 只当顺序时最好 */
#define	ACL_SLICE_FLAG_RTGC_OFF		(1 << 10) /**< 关闭实时内存释放 */
#define	ACL_SLICE_FLAG_LP64_ALIGN	(1 << 11) /**< 是否针对64位平台需要按8字节对齐 */

/**
 * 内存分片池的状态结构
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
 * 创建内存片池对象
 * @param name {const char*} 标识名称，以便于调试
 * @param page_size {int} 分配内存时的分配内存页大小
 * @param slice_size {int} 每个固定长度内存片的大小
 * @param flag {unsigned int} 标志位，参见上述：ACL_SLICE_FLAG_xxx
 * @return {ACL_SLICE*} 内存片池对象句柄
 */
ACL_API ACL_SLICE *acl_slice_create(const char *name, int page_size,
	int slice_size, unsigned int flag);

/**
 * 销毁一个内存片池对象
 * @param slice {ACL_SLICE*} 内存片池对象
 */
ACL_API void acl_slice_destroy(ACL_SLICE *slice);

/**
 * 该内存片池中有多少个内存片正在被使用
 * @param slice {ACL_SLICE*} 内存片池对象
 * @return {int} >= 0, 正在被使用的内存片个数
 */
ACL_API int acl_slice_used(ACL_SLICE *slice);

/**
 * 分配一块内存片
 * @param slice {ACL_SLICE*} 内存片池对象
 * @return {void*} 内存片地址
 */
ACL_API void *acl_slice_alloc(ACL_SLICE *slice);

/**
 * 分配一块内存片，且将内存片内容初始化为0
 * @param slice {ACL_SLICE*} 内存片池对象
 * @return {void*} 内存片地址
 */
ACL_API void *acl_slice_calloc(ACL_SLICE *slice);

/**
 * 释放一块内存片
 * @param slice {ACL_SLICE*} 内存片池对象
 * @param ptr {void*} 内存片地址, 必须是由 acl_slice_alloc/acl_slice_calloc 所分配
 */
ACL_API void acl_slice_free2(ACL_SLICE *slice, void *ptr);

/**
 * 释放一块内存片
 * @param ptr {void*} 内存片地址, 必须是由 acl_slice_alloc/acl_slice_calloc 所分配
 */
ACL_API void acl_slice_free(void *ptr);

/**
 * 查看内存片池的当前状态
 * @param slice {ACL_SLICE*} 内存片池对象
 * @param sbuf {ACL_SLICE_STAT*} 存储结果, 不能为空
 */
ACL_API void acl_slice_stat(ACL_SLICE *slice, ACL_SLICE_STAT *sbuf);

/**
 * 手工将内存片池不用的内存进行释放
 * @param slice {ACL_SLICE*} 内存片池对象
 */
ACL_API int acl_slice_gc(ACL_SLICE *slice);

/*----------------------------------------------------------------------------*/

typedef struct ACL_SLICE_POOL {
	ACL_SLICE **slices;		/* the slice array */
	int   base;			/* the base byte size */
	int   nslice;			/* the max number of base size */
	unsigned int slice_flag;	/* flag: ACL_SLICE_FLAG_GC2[3] | ACL_SLICE_FLAG_RTGC_OFF */
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
