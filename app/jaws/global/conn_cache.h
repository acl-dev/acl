#ifndef	__CONN_CACHE_INCLUDE_H__
#define	__CONN_CACHE_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct CONN_CACHE {
	ACL_AIO *aio;
	ACL_HTABLE *cache;
	int   conn_limit;
	int   nset;
	int   nget;
	int   nclose;
} CONN_CACHE;

typedef struct CONN CONN;

/**
 * 连接池初始化函数, 该函数仅在程序初始化时被调用一次
 * @param aio {ACL_AIO*} 异步流对象
 * @param conn_limit {int} 每一个连接池的数量限制
 * @return {CONN_CACHE*} 创建一个连接池的缓存对象
 */
CONN_CACHE *conn_cache_create(ACL_AIO *aio, int conn_limit);

/**
 * 向连接池中添加一个连接流
 * @param cache {CONN_CAHCE*} 长连接缓存对象
 * @param timeout {int} 该连接的超时时间
 * @param free_fn {void (*)(ACL_ASTREAM*, void*)} 关闭连接时的回调函数,
 *  如果该参数非空，则当该连接被关闭前会自动调用 free_fn
 * @param ctx {void*} free_fn 的参数之一
 */
void conn_cache_push_stream(CONN_CACHE *cache, ACL_ASTREAM *stream,
	int timeout, void (*free_fn)(ACL_ASTREAM*, void*), void *ctx);

/**
 * 从连接池取出对应某个键值的连接对象
 * @param cache {CONN_CAHCE*} 长连接缓存对象
 * @param key {const char*} 查询键值，如：192.168.0.1:80
 * @return {CONN*} 连接对象, 若为 NULL 则表示不存在
 */
CONN *conn_cache_get_conn(CONN_CACHE *cache, const char *key);

/**
 * 从连接池取出对应某个键值的连接流
 * @param cache {CONN_CAHCE*} 长连接缓存对象
 * @param key {const char*} 查询键值，如：192.168.0.1:80
 * @param ctx_pptr {void**} 如果结果非空，则存储用户的自定义变量
 * @return {ACL_ASTREAM*} 连接流
 */
ACL_ASTREAM *conn_cache_get_stream(CONN_CACHE *cache, const char *key, void **ctx_pptr);

/**
 * 从连接池中删除对应某个键值的连接
 * @param cache {CONN_CAHCE*} 长连接缓存对象
 * @param key {const char*} 查询键值，如：192.168.0.1:80
 */
void conn_cache_delete_key(CONN_CACHE *cache, const char *key);

/**
 * 从连接池中删除某个连接对象且关闭连接流
 * @param conn {CONN*} 连接对象
 */
void conn_cache_delete_conn(CONN *conn);

/**
 * 从连接池中删除某个连接流并删除对应的连接对象且关闭连接流
 * @param cache {CONN_CAHCE*} 长连接缓存对象
 * @param stream {ACL_ASTREAM*} 连接流
 */
void conn_cache_delete_stream(CONN_CACHE *cache, ACL_ASTREAM *stream);

#ifdef	__cplusplus
}
#endif

#endif
