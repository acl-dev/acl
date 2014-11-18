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
 * 创建一个内存分配池对象
 * @param mem_limit {size_t} 内存池的最大内存，单位为字节
 * @return {ACL_ALLOCATOR *} 内存分配池对象指针
 */
ACL_API ACL_ALLOCATOR *acl_allocator_create(size_t mem_limit);

/**
 * 控制内存分配池的一些参数
 * @param name {int} 参数列表的第一个参数
 * 调用方式如下：
 * acl_allocator_ctl(ACL_ALLOCATOR_CTL_MIN_SIZE, 128,
 *		ACL_ALLOCATOR_CTL_MAX_SIZE, 1024,
 *		ACL_ALLOCATOR_CTL_END);
 */
ACL_API void acl_allocator_ctl(int name, ...);

#define ACL_ALLOCATOR_CTL_END		0    /**< 结束标记 */
#define ACL_ALLOCATOR_CTL_MIN_SIZE	1    /**< 设置最小字节数 */
#define ACL_ALLOCATOR_CTL_MAX_SIZE	2    /**< 设置最大字节数 */

/**
 * 配置内存分配池的容量大小
 * @param allocator {ACL_ALLOCATOR*}
 * @param mem_limit {size_t} 内存池的最大值，单位为字节
 */
ACL_API void acl_allocator_config(ACL_ALLOCATOR *allocator, size_t mem_limit);

/**
 * 释放内存分配池对象及其所管理的内存
 * @param allocator {ACL_ALLOCATOR*}
 */
ACL_API void acl_allocator_free(ACL_ALLOCATOR *allocator);

/**
 * 添加一个新的内存分配类型
 * @param allocator {ACL_ALLOCATOR*}
 * @param label {const char*} 该内存分配类型的描述信息
 * @param obj_size {size_t} 每个该内存类型的大小，单位为字节
 * @param type {acl_mem_type} 内存类型
 * @param after_alloc_fn {void (*)(void*, void*)} 分配内存成功后调用的函数，可以为空
 * @param before_free_fn {void (*)(void*, void*)} 释放内存前回调的函数，可以为空
 * @param pool_ctx {void*} 应用自己的私有对象，如果 after_alloc_fn 或 before_free_fn
 *        不为空，则回调时将此参数直接传递给应用
 * @return {ACL_MEM_POOL*} 该内存分配类型所对应的对象
 */
ACL_API ACL_MEM_POOL *acl_allocator_pool_add(ACL_ALLOCATOR *allocator,
					const char *label,
					size_t obj_size,
					acl_mem_type type,
					void (*after_alloc_fn)(void *obj, void *pool_ctx),
					void (*before_free_fn)(void *obj, void *pool_ctx),
					void *pool_ctx);

/**
 * 从内存分配池中移除某种内存分配类型
 * @param allocator {ACL_ALLOCATOR*}
 * @param pool {ACL_MEM_POOL*} 由 acl_allocatore_pool_add 返回的对象
 */
ACL_API void acl_allocator_pool_remove(ACL_ALLOCATOR *allocator, ACL_MEM_POOL *pool);

/**
 * 探测某种分配类型是否存在于内存分配池的内存分配类型中
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} 内存类型
 * @return {int}, 0: 否，!= 0: 是
 */
ACL_API int acl_allocator_pool_ifused(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * 某种分配类型的内存对象当前被使用的个数
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} 内存类型
 * @return {int} 当前正在被使用的某种内存分配类型的内存对象个数
 */
ACL_API int acl_allocator_pool_inuse_count(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * 某种分配类型所分配的内存中当前正在被使用的内存大小
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} 内存类型
 * @return {int} 某种分配类型所分配的内存中当前正在被使用的内存大小，单位为字节
 */
ACL_API int acl_allocator_pool_inuse_size(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * 内存分配池总共分配的且正在被使用的内存的大小
 * @param allocator {ACL_ALLOCATOR*}
 * @return {int} 内存大小，单位：字节
 */
ACL_API int acl_allocator_pool_total_allocated(ACL_ALLOCATOR *allocator);

/**
 * 分配某种内存类型的内存
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} 内存类型
 * @return {void*} 新分配的内存的地址
 */
ACL_API void *acl_allocator_mem_alloc(ACL_ALLOCATOR *allocator, acl_mem_type type);

/**
 * 释放某种内存类型的内存空间
 * @param allocator {ACL_ALLOCATOR*}
 * @param type {acl_mem_type} 内存类型
 * @param obj {void*} 被释放的内存对象，不能为空
 */
ACL_API void acl_allocator_mem_free(ACL_ALLOCATOR *allocator, acl_mem_type type, void *obj);

/**
 * 根据所要求的内存大小，自动进行内存分配类型匹配，若找到所匹配的类型，则采用内存池
 * 的内存分配策略，否则直接调用 acl_mymalloc 进行内存分配
 * @param filename {const char*} 调用本函数的当前文件名
 * @param line {int} 调用本函数的当前文件行号
 * @param allocator {ACL_ALLOCATOR*}
 * @param size {size_t} 调用者所申请的内存大小
 * @return {void*} 新分配的内存的地址
 */
ACL_API void *acl_allocator_membuf_alloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, size_t size);

/**
 * 根据所申请的内存大小，重新分配内存空间，若找到所匹配的类型，则采用内存池
 * 内存分配策略，否则直播调用 acl_mymalloc 进行内存分配
 * @param filename {const char*} 调用本函数的当前文件名
 * @param line {int} 调用本函数的当前文件行号
 * @param allocator {ACL_ALLOCATOR*}
 * @param oldbuf {void*} 原来分配的内存
 * @param size {size_t} 本次申请的内存大小
 * @return {void*} 新分配的内存的地址
 */
ACL_API void *acl_allocator_membuf_realloc(const char *filename, int line,
	ACL_ALLOCATOR *allocator, void *oldbuf, size_t size);

/**
 * 释放内存, 如果能找到该大小的内存所属的内存分配类型，则进行缓冲，否则直播调用
 * acl_myfree 进行释放
 * @param filename {const char*} 调用本函数的当前文件名
 * @param line {int} 调用本函数的当前文件行号
 * @param allocator {ACL_ALLOCATOR*}
 * @param buf {void*} 内存地址
 */
ACL_API void acl_allocator_membuf_free(const char *filename, int line,
	ACL_ALLOCATOR *allocator, void *buf);

#ifdef __cplusplus
}
#endif

#endif
