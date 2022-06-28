#ifndef	ACL_CACHE2_INCLUDE_H
#define	ACL_CACHE2_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif
#include "acl_define.h"
#include <time.h>

typedef struct ACL_CACHE2 ACL_CACHE2;

/**
 * 缓存池中存储的缓存对象
 */
typedef struct ACL_CACHE2_INFO {
	char  *key;		/**< 健值 */
	void  *value;		/**< 用户动态对象 */
	int    nrefer;		/**< 引用计数 */
	time_t when_timeout;	/**< 过期时间截 */
	ACL_CACHE2* cache;	/**< 引用缓存对象 */
} ACL_CACHE2_INFO;

/**
 * 缓冲池
 */
struct ACL_CACHE2 { 
	int   max_size;		/**< 缓存池容量大小限制值 */
	int   size;		/**< 当前缓存池中的缓存对象个数 */
	void *ctx;		/**< 外部引用对象 */

	/**< 释放用户动态对象的释放回调函数 */
	void  (*free_fn)(const ACL_CACHE2_INFO*, void *);

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_CACHE2*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_CACHE2*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_CACHE2*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_CACHE2*);
	/* 取迭代器关联的当前容器成员结构对象 */
	ACL_CACHE2_INFO *(*iter_info)(ACL_ITER*, struct ACL_CACHE2*);
};

/**
 * 创建一个缓存池，并设置每个缓存对象的最大缓存时长及该缓存池的空间容量限制
 * @param max_size {int} 该缓存池的容量限制，若该值 <= 0，则内部不会限制缓存量
 * @param free_fn {void (*)(void*)} 用户级的释放缓存对象的函数
 * @return {ACL_CACHE2*} 缓存池对象句柄
 */
ACL_API ACL_CACHE2 *acl_cache2_create(int max_size,
	void (*free_fn)(const ACL_CACHE2_INFO*, void*));

/**
 * 释放一个缓存池
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 */
ACL_API void acl_cache2_free(ACL_CACHE2 *cache2);

/**
 * 向缓存池中添加被缓存的对象
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*} 缓存对象的健值
 * @param value {void*} 动态缓存对象
 * @param timeout {int} 每个缓存对象的缓存时长
 * @return {ACL_CACHE2_INFO*} 缓存对象所依附的结构对象，其中的 value 与用户的
 *  对象相同, 如果返回 NULL 则表示添加失败，失败原因为：缓存池太大溢出或相同
 *  健值的对象存在且引用计数非0; 如果返回非 NULL 则表示添加成功，如果对同一健
 *  值的重复添加，会用新的数据替换旧的数据，且旧数据调用释放函数进行释放
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_enter(ACL_CACHE2 *cache2,
	const char *key, void *value, int timeout);

/**
 * 从缓存池中查找某个被缓存的对象
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*} 查询健
 * @return {void*} 被缓存的用户对象的地址，为NULL时表示未找到
 */
ACL_API void *acl_cache2_find(ACL_CACHE2 *cache2, const char *key);

/**
 * 从缓存池中查找某个被缓存的对象所依附的缓存信息对象
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*} 查询健
 * @return {ACL_CACHE2_INFO*} 缓存信息对象地址，为NULL时表示未找到
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_locate(ACL_CACHE2 *cache2, const char *key);

/**
 * 从缓存池中删除某个缓存对象
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param info {ACL_CACHE2_INFO*} 用户对象所依附的缓存信息对象
 * @return {int} 0: 表示删除成功; -1: 表示该对象的引用计数非0或该对象不存在
 */
ACL_API int acl_cache2_delete(ACL_CACHE2 *cache2, ACL_CACHE2_INFO *info);

/**
 * 从缓存池中删除某个缓存对象
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*} 健值
 * @return {int} 0: 表示删除成功; -1: 表示该对象的引用计数非0或该对象不存在
 */
ACL_API int acl_cache2_delete2(ACL_CACHE2 *cache2, const char *key);

/**
 * 使缓存池中的过期对象被自动删除
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @return {int} >= 0: 被自动删除的缓存对象的个数
 */
ACL_API int acl_cache2_timeout(ACL_CACHE2 *cache2);

/**
 * 使某个缓存对象的缓存时间加长
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param info {ACL_CACHE2_INFO*} 缓存对象
 * @param timeout {int} 缓存时长(秒)
 * @return {ACL_CACHE2_INFO*} 返回非 NULL 表示正常，为 NULL 表示 key 不存在
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_update2(ACL_CACHE2 *cache2,
	ACL_CACHE2_INFO *info, int timeout);

/**
 * 使某个缓存对象的缓存时间加长
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*} 健值
 * @param timeout {int} 缓存时长(秒)
 * @return {ACL_CACHE2_INFO*} 返回非 NULL 表示正常，为 NULL 表示 key 不存在
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_update(ACL_CACHE2 *cache2,
	const char *key, int timeout);

/**
 * 添加或更新缓存中的对象
 * @param key {const char*} 缓存对象的健值
 * @param value {void*} 动态缓存对象
 * @param timeout {int} 每个缓存对象的缓存时长
 * @param exist {int*} 非 NULL 时，如果新 key 存在则被置 1，否则置 0
 * @return {ACL_CACHE2_INFO*} 缓存对象所依附的结构对象，其中的 value 与用户的
 *  对象相同；如果该 key 不存在则添加新对象，如果存在则更新旧对象；返回 NULL
 *  表示更新添加失败；如果是更新方式，则调用者应注意释放临时创建的动态对象
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_upsert(ACL_CACHE2 *cache2,
	const char *key, void *value, int timeout, int *exist);
	
/**
 * 获取按时间排序后的头部对象，调用者可从 ACL_CACHE2_INFO::value 获得应用对象
 * @param cache2 {ACL_CACHE2*}
 * @return {ACL_CACHE2_INFO*} 返回 NULL 表示缓存对象为空
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_head(ACL_CACHE2 *cache2);

/**
 * 获取按时间排序后的尾部对象，调用者可从 ACL_CACHE2_INFO::value 获得应用对象
 * @param cache2 {ACL_CACHE2*}
 * @return {ACL_CACHE2_INFO*} 返回 NULL 表示缓存对象为空
 */
ACL_API ACL_CACHE2_INFO *acl_cache2_tail(ACL_CACHE2 *cache2);

/**
 * 增加某缓存对象的引用计数，防止被提前删除
 * @param info {ACL_CACHE2_INFO*} 用户对象所依附的缓存信息对象
 */
ACL_API void acl_cache2_refer(ACL_CACHE2_INFO *info);

/**
 * 增加某缓存对象的引用计数，防止被提前删除
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*}
 */
ACL_API void acl_cache2_refer2(ACL_CACHE2 *cache2, const char *key);

/**
 * 减少某缓存对象的引用计数
 * @param info {ACL_CACHE2_INFO*} 用户对象所依附的缓存信息对象
 */
ACL_API void acl_cache2_unrefer(ACL_CACHE2_INFO *info);

/**
 * 减少某缓存对象的引用计数
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param key {const char*}
 */
ACL_API void acl_cache2_unrefer2(ACL_CACHE2 *cache2, const char *key);

/**
 * 加锁缓存池对象，在多线程时用
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 */
ACL_API void acl_cache2_lock(ACL_CACHE2 *cache2);

/**
 * 解锁缓存池对象，在多线程时用
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 */
ACL_API void acl_cache2_unlock(ACL_CACHE2 *cache2);

/**
 * 遍历缓存中的所有对象
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param walk_fn {void (*)(ACL_CACHE2_INFO*, void*)} 遍历回调函数
 * @param arg {void *} walk_fn()/2 中的第二个参数
 */
ACL_API void acl_cache2_walk(ACL_CACHE2 *cache2,
	void (*walk_fn)(ACL_CACHE2_INFO *, void *), void *arg);

/**
 * 清空缓存池中的缓存对象，如果某个缓存对象依然在被引用且非强制性删除，则将不会被清空
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @param force {int} 如果非0，则即使某个缓存对象的引用计数非0也会被删除
 * @return {int} 被清除的缓存对象个数
 */
ACL_API int acl_cache2_clean(ACL_CACHE2 *cache2, int force);

/**
 * 当前缓存池中缓存对象的个数
 * @param cache2 {ACL_CACHE2*} 缓存池对象句柄
 * @return {int} 被缓存的对象个数
 */
ACL_API int acl_cache2_size(ACL_CACHE2 *cache2);

#ifdef	__cplusplus
}
#endif

#endif
