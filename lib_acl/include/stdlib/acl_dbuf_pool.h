#ifndef	ACL_DBUF_POOL_INCLUDE_H
#define	ACL_DBUF_POOL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ACL_DBUF_POOL ACL_DBUF_POOL;

/**
 * 创建内存池对象
 * @param block_size {size_t} 内存池中每个连续内存块的大小（字节）
 * @return {ACL_DBUF_POOL*} 返回非 NULL 对象
 */
ACL_API ACL_DBUF_POOL *acl_dbuf_pool_create(size_t block_size);

/**
 * 重置内存池状态，会将多余的内存数据块释放
 * @param pool {ACL_DBUF_POOL*} 内存池对象
 * @param off {size_t} 要求保留的最小内存相对偏移位置
 * @return {int} 返回 0 表示操作成功，非 0 表示失败
 */
ACL_API int  acl_dbuf_pool_reset(ACL_DBUF_POOL *pool, size_t off);

/**
 * 销毁内存池
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 */
ACL_API void acl_dbuf_pool_destroy(ACL_DBUF_POOL *pool);

/**
 * 分配指定长度的内存
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param  len {size_t} 需要分配的内存大小
 * @return {void*} 新分配的内存地址
 */
ACL_API void *acl_dbuf_pool_alloc(ACL_DBUF_POOL *pool, size_t len);

/**
 * 分配指定长度的内存并将内存区域清零
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param len {size_t} 需要分配的内存长度
 * @return {void*} 新分配的内存地址
 */
ACL_API void *acl_dbuf_pool_calloc(ACL_DBUF_POOL *pool, size_t len);

/**
 * 根据输入的字符串动态创建新的内存并将字符串进行复制，类似于 strdup
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param s {const char*} 源字符串
 * @return {char*} 新复制的字符串地址
 */
ACL_API char *acl_dbuf_pool_strdup(ACL_DBUF_POOL *pool, const char *s);

/**
 * 根据输入的字符串动态创建新的内存并将字符串进行复制，类似于 strdup
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param s {const char*} 源字符串
 * @param len {size_t} 限定的最大字符串长度
 * @return {char*} 新复制的字符串地址
 */
ACL_API char *acl_dbuf_pool_strndup(ACL_DBUF_POOL *pool,
	const char *s, size_t len);

/**
 * 根据输入的内存数据动态创建内存并将数据进行复制
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param addr {const void*} 源数据内存地址
 * @param len {size_t} 源数据长度
 * @return {void*} 新复制的数据地址
 */
ACL_API void *acl_dbuf_pool_memdup(ACL_DBUF_POOL *pool,
		const void *addr, size_t len);

/**
 * 归还由内存池分配的内存
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param addr {const void*} 由内存池分配的内存地址
 * @return {int} 如果该内存地址非内存池分配或释放多次，则返回 -1，操作成功则
 *  返回 0
 */
ACL_API int acl_dbuf_pool_free(ACL_DBUF_POOL *pool, const void *addr);

/**
 * 保留由内存池分配的某段地址，以免当调用 reset 时被提前释放掉
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param addr {const void*} 由内存池分配的内存地址
 * @return {int} 操作成功则返回 0，如果该内存地址非内存池分配，则返回 -1
 */
ACL_API int acl_dbuf_pool_keep(ACL_DBUF_POOL *pool, const void *addr);

/**
 * 取消保留由内存池分配的某段地址，以便于调用 dbuf_reset 时被释放掉
 * @param pool {ACL_DBUF_POOL*} 对象池对象
 * @param addr {const void*} 由内存池分配的内存地址
 * @return {int} 操作成功则返回 0，如果该内存地址非内存池分配，则返回 -1
 */
ACL_API int acl_dbuf_pool_unkeep(ACL_DBUF_POOL *pool, const void *addr);

/**
 * 内部测试用函数
 */
ACL_API void acl_dbuf_pool_test(size_t max);

#ifdef	__cplusplus
}
#endif

#endif
