#ifndef	ACL_CACHE_INCLUDE_H
#define	ACL_CACHE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif
#include "acl_define.h"
#include "acl_htable.h"
#include "acl_ring.h"
#include <time.h>

/**
 * 缓存池中存储的缓存对象
 */
typedef struct ACL_CACHE_INFO {
	char *key;		/**< 健值 */
	void *value;		/**< 用户动态对象 */
	int   nrefer;		/**< 引用计数 */
	time_t when_timeout;	/**< 过期时间截 */
	ACL_RING entry;		/**< 内部数据链成员 */
} ACL_CACHE_INFO;

/**
 * 缓冲池
 */
typedef struct ACL_CACHE { 
	ACL_HTABLE *table;	/**< 哈希表 */
	ACL_RING ring;		/**< 将被删除的对象的数据链表 */
	int   max_size;		/**< 缓存池容量大小限制值 */
	int   size;		/**< 当前缓存池中的缓存对象个数 */
	int   timeout;		/**< 每个缓存对象的生存时长(秒) */

	/**< 释放用户动态对象的释放回调函数 */
	void  (*free_fn)(const ACL_CACHE_INFO*, void *);
	acl_pthread_mutex_t lock;	/**< 缓存池锁 */
	ACL_SLICE *slice;		/**< 内存切片对象 */

	/* for acl_iterator */

	/* 取迭代器头函数 */
	void *(*iter_head)(ACL_ITER*, struct ACL_CACHE*);
	/* 取迭代器下一个函数 */
	void *(*iter_next)(ACL_ITER*, struct ACL_CACHE*);
	/* 取迭代器尾函数 */
	void *(*iter_tail)(ACL_ITER*, struct ACL_CACHE*);
	/* 取迭代器上一个函数 */
	void *(*iter_prev)(ACL_ITER*, struct ACL_CACHE*);
	/* 取迭代器关联的当前容器成员结构对象 */
	ACL_CACHE_INFO *(*iter_info)(ACL_ITER*, struct ACL_CACHE*);
} ACL_CACHE;

/**
 * 创建一个缓存池，并设置每个缓存对象的最大缓存时长及该缓存池的空间容量限制
 * @param max_size {int} 该缓存池的容量限制
 * @param timeout {int} 每个缓存对象的缓存时长
 * @param free_fn {void (*)(void*)} 用户级的释放缓存对象的函数
 * @return {ACL_CACHE*} 缓存池对象句柄
 */
ACL_API ACL_CACHE *acl_cache_create(int max_size, int timeout,
	void (*free_fn)(const ACL_CACHE_INFO*, void*));

/**
 * 释放一个缓存池，并自动调用 acl_cache_create()/3 中的释放函数释放缓存对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 */
ACL_API void acl_cache_free(ACL_CACHE *cache);

/**
 * 向缓存池中添加被缓存的对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*} 缓存对象的健值
 * @param value {void*} 动态缓存对象
 * @return {ACL_CACHE_INFO*} 缓存对象所依附的结构对象，其中的 value 与用户的对象相同,
 *   如果返回 NULL 则表示添加失败，失败原因为：缓存池太大溢出或相同健值的对象存在
 *   且引用计数非0; 如果返回非 NULL 则表示添加成功，如果对同一健值的重复添加，会用
 *   新的数据替换旧的数据，且旧数据调用释放函数进行释放
 */
ACL_API ACL_CACHE_INFO *acl_cache_enter(ACL_CACHE *cache, const char *key, void *value);

/**
 * 从缓存池中查找某个被缓存的对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*} 查询健
 * @return {void*} 被缓存的用户对象的地址，为NULL时表示未找到
 */
ACL_API void *acl_cache_find(ACL_CACHE *cache, const char *key);

/**
 * 从缓存池中查找某个被缓存的对象所依附的缓存信息对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*} 查询健
 * @return {ACL_CACHE_INFO*} 缓存信息对象地址，为NULL时表示未找到
 */
ACL_API ACL_CACHE_INFO *acl_cache_locate(ACL_CACHE *cache, const char *key);

/**
 * 从缓存池中删除某个缓存对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param info {ACL_CACHE_INFO*} 用户对象所依附的缓存信息对象
 * @return {int} 0: 表示删除成功; -1: 表示该对象的引用计数非0或该对象不存在
 */
ACL_API int acl_cache_delete(ACL_CACHE *cache, ACL_CACHE_INFO *info);

/**
 * 从缓存池中删除某个缓存对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*} 健值
 * @return {int} 0: 表示删除成功; -1: 表示该对象的引用计数非0或该对象不存在
 */
ACL_API int acl_cache_delete2(ACL_CACHE *cache, const char *key);

/**
 * 使缓存池中的过期对象被自动删除
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @return {int} >= 0: 被自动删除的缓存对象的个数
 */
ACL_API int acl_cache_timeout(ACL_CACHE *cache);

/**
 * 使某个缓存对象的缓存时间加长
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param info {ACL_CACHE_INFO*} 缓存对象
 * @param timeout {int} 缓存时长(秒)
 */
ACL_API void acl_cache_update2(ACL_CACHE *cache, ACL_CACHE_INFO *info, int timeout);

/**
 * 使某个缓存对象的缓存时间加长
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*} 健值
 * @param timeout {int} 缓存时长(秒)
 */
ACL_API void acl_cache_update(ACL_CACHE *cache, const char *key, int timeout);

/**
 * 增加某缓存对象的引用计数，防止被提前删除
 * @param info {ACL_CACHE_INFO*} 用户对象所依附的缓存信息对象
 */
ACL_API void acl_cache_refer(ACL_CACHE_INFO *info);

/**
 * 增加某缓存对象的引用计数，防止被提前删除
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*}
 */
ACL_API void acl_cache_refer2(ACL_CACHE *cache, const char *key);

/**
 * 减少某缓存对象的引用计数
 * @param info {ACL_CACHE_INFO*} 用户对象所依附的缓存信息对象
 */
ACL_API void acl_cache_unrefer(ACL_CACHE_INFO *info);

/**
 * 减少某缓存对象的引用计数
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param key {const char*}
 */
ACL_API void acl_cache_unrefer2(ACL_CACHE *cache, const char *key);

/**
 * 加锁缓存池对象，在多线程时用
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 */
ACL_API void acl_cache_lock(ACL_CACHE *cache);

/**
 * 解锁缓存池对象，在多线程时用
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 */
ACL_API void acl_cache_unlock(ACL_CACHE *cache);

/**
 * 遍历缓存中的所有对象
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param walk_fn {void (*)(ACL_CACHE_INFO*, void*)} 遍历回调函数
 * @param arg {void *} walk_fn()/2 中的第二个参数
 */
ACL_API void acl_cache_walk(ACL_CACHE *cache, void (*walk_fn)(ACL_CACHE_INFO *, void *), void *arg);

/**
 * 清空缓存池中的缓存对象，如果某个缓存对象依然在被引用且非强制性删除，则将不会被清空
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @param force {int} 如果非0，则即使某个缓存对象的引用计数非0也会被删除
 * @return {int} 被清除的缓存对象个数
 */
ACL_API int acl_cache_clean(ACL_CACHE *cache, int force);

/**
 * 当前缓存池中缓存对象的个数
 * @param cache {ACL_CACHE*} 缓存池对象句柄
 * @return {int} 被缓存的对象个数
 */
ACL_API int acl_cache_size(ACL_CACHE *cache);

#ifdef	__cplusplus
}
#endif

#endif
