#include "lib_acl.h"
#include "conn_cache.h"

/* 对应某个 IP:PORT 键值的连接池结构类型定义 */

typedef struct CONN_POOL {
	CONN_CACHE *conn_cache;	/* 所从属的某个连接池缓存对象 */
	ACL_AIO *aio;		/* 该连接池所从属的异步IO对象 */
	ACL_FIFO conns;		/* 该连接池中的连接对象(CONN)队列 */
	char key[256];		/* 该连接池对象所对应的存储于连接池缓存中的存储键 */
} CONN_POOL;

/* 连接池中的某个连接对象类型定义 */

struct CONN {
	CONN_POOL *conn_pool;	/* 所从属的的连接池对象 */
	ACL_ASTREAM *stream;	/* 该连接的异步流对象 */
	void (*free_fn)(ACL_ASTREAM *stream, void*); /* 释放该连接时的回调函数 */
	void *ctx;		/* free_fn 对参数之一 */
	ACL_FIFO_INFO *info;	/* 该连接对象存储于连接池(conn_pool)的队列中的对象指针 */
};

/* 仅释放连接对象所占用的内存，但并不关闭连接流 */

static void conn_free(CONN *conn)
{
	CONN_POOL *conns = conn->conn_pool;

	/* 先从连接池队列中删除 */
	if (conn->info)
		acl_fifo_delete_info(&conns->conns, conn->info);
	/* 调用用户自定义回调函数 */
	if (conn->free_fn)
		conn->free_fn(conn->stream, conn->ctx);
	/* 释放内存空间 */
	acl_myfree(conn);
}

/* 释放连接对象并关闭连接流 */

static void conn_close(CONN *conn)
{
	/* 必须首先禁止异步流的读监听 */
	acl_aio_disable_read(conn->stream);

	/* 异步关闭该连接, 然后由异步框架自动触发 read_close_callback */
	acl_aio_iocp_close(conn->stream);
}

/* 当连接池对象被释放时调用此回调函数 */

static void conn_pool_free(CONN_POOL *conns)
{
	CONN *conn;

	/* 需要把连接池中的所有连接都释放 */

	while ((conn = acl_fifo_pop(&conns->conns)) != NULL) {
		if (conn->stream) {
			if (conn->free_fn)
				conn->free_fn(conn->stream, conn->ctx);
			acl_aio_clean_hooks(conn->stream);
			acl_aio_iocp_close(conn->stream);
		}
		acl_myfree(conn);
	}
	acl_myfree(conns);
}

/* 释放连接池定时器回调函数 */

static void conn_pool_free_timer(int event_type acl_unused, void *context)
{
	CONN_POOL *conns = (CONN_POOL*) context;

	conn_pool_free(conns);
}

/* 设置释放连接池的定时器, 之所以采用定时器来释放连接池对象是为了
 * 使释放过程不在事务的递归处理过程中被提前释放
 */

static void set_conn_pool_free_timer(CONN_POOL *conns)
{
	acl_aio_request_timer(conns->aio, conn_pool_free_timer, conns, 1, 1);
}

static void conn_pool_stat_timer(int event_type acl_unused, void *context)
{
	const char *myname = "conn_pool_stat_timer";
	CONN_CACHE *cache = (CONN_CACHE*) context;

	/* only for test */
	if (0)
	acl_msg_info("%s(%d): nset: %d, nget: %d, nclose: %d, inter: %d",
		myname, __LINE__, cache->nset, cache->nget,
		cache->nclose, cache->nset - cache->nget - cache->nclose);
}

static void set_conn_pool_stat_timer(CONN_CACHE *cache)
{
	/* 设置定时器 */
	acl_aio_request_timer(cache->aio, conn_pool_stat_timer, cache, 2, 1);
}

CONN_CACHE *conn_cache_create(ACL_AIO *aio, int conn_limit)
{
	const char *myname = "conn_cache_create";
	CONN_CACHE *cache = (CONN_CACHE*) acl_mycalloc(1, sizeof(CONN_CACHE));

	cache->aio = aio;
	cache->conn_limit = conn_limit;
	cache->cache = acl_htable_create(1024, 0);
	acl_msg_info("%s(%d): ok, conn_limit: %d", myname, __LINE__, conn_limit);

	/* 设置连接池缓存状态信息的定时器 */
	set_conn_pool_stat_timer(cache);
	return (cache);
}

/* 流可读时的回调函数 */

static int read_callback(ACL_ASTREAM *stream acl_unused, void *ctx acl_unused,
	char *data acl_unused, int dlen acl_unused)
{
	const char *myname = "read_callback";

	acl_msg_info("%s(%d), %s: can read connection from server, dlen(%d), data(%s)",
		__FILE__, __LINE__, myname, dlen, data);

	/* 因为该连接为空闲连接，不应有数据可读，如果有数据可读，则因为
	 * 无法知道如何处理这些数据而需要关闭该连接
	 */

	/* 返回 -1 从而触发关闭回调函数 */
	return (-1);
}

/* 流关闭时的回调函数 */

static int read_close_callback(ACL_ASTREAM *stream acl_unused, void *ctx)
{
	CONN *conn = (CONN*) ctx;
	CONN_POOL *conns = conn->conn_pool;

	/* 释放该连接对象的内存空间，但并不关闭该连接，
	 * 关闭过程由异步框架自动关闭
	 */
	conn_free(conn);

	/* 如果连接池为空则释放该连接池 */
	if (acl_fifo_size(&conns->conns) == 0) {
		acl_htable_delete(conns->conn_cache->cache, conns->key, NULL);
		set_conn_pool_free_timer(conns);
	}

	conns->conn_cache->nclose++;

	/* 触发 acl_aio_iocp_close 过程 */
	return (-1);
}

/* 流读超时时的回调函数 */

static int read_timeout_callback(ACL_ASTREAM *stream acl_unused, void *ctx acl_unused)
{
	/* 返回 -1 从而触发关闭回调函数 */
	return (-1);
}
	
void conn_cache_push_stream(CONN_CACHE *cache, ACL_ASTREAM *stream,
	int timeout, void (*free_fn)(ACL_ASTREAM*, void*), void *ctx)
{
	const char *key = ACL_VSTREAM_PEER(acl_aio_vstream(stream));
	CONN_POOL *conns;
	CONN *conn;

#if 0
	acl_aio_clean_hooks(stream);
#endif

	/* 查看该KEY的连接池句柄是否存在，如果存在则复用，否则创建新的 */

	conns = (CONN_POOL*) acl_htable_find(cache->cache, key);
	if (conns == NULL) {
		conns = (CONN_POOL*) acl_mymalloc(sizeof(CONN_POOL));
		conns->conn_cache = cache;
		conns->aio = acl_aio_handle(stream);
		acl_fifo_init(&conns->conns);
		ACL_SAFE_STRNCPY(conns->key, key, sizeof(conns->key));
		acl_htable_enter(cache->cache, key, conns);
	}

#if 0
	/* 如果该连接池中的连接流超过限制，则优先释放最旧的连接对象 */
	if (acl_fifo_size(&conns->conns) >= cache->conn_limit) {
		conn = acl_fifo_pop(&conns->conns);
		if (conn) {
			conn->info = NULL;
			conn_close(conn);
		}
	}
#endif

	cache->nset++;

	/* 创建新的异步流连接缓存对象 */
	conn = (CONN*) acl_mymalloc(sizeof(CONN));
	conn->stream = stream;
	conn->ctx = ctx;
	conn->free_fn = free_fn;

	/* 加入流连接池中 */
	conn->info = acl_fifo_push(&conns->conns, conn);
	conn->conn_pool = conns;

	/* 设置该流的回调函数 */
	acl_aio_ctl(stream,
		ACL_AIO_CTL_READ_HOOK_ADD, read_callback, conn,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, read_close_callback, conn,
		ACL_AIO_CTL_TIMEO_HOOK_ADD, read_timeout_callback, conn,
		ACL_AIO_CTL_TIMEOUT, timeout,
		ACL_AIO_CTL_END);
	/* 开始读该流的数据 */
	acl_aio_read(stream);
}

CONN *conn_cache_get_conn(CONN_CACHE *cache, const char *key)
{
	CONN_POOL *conns;
	CONN *conn;

	/* 先查看该KEY的连接池对象是否存在，如果不存在则返回NULL */

	conns = (CONN_POOL*) acl_htable_find(cache->cache, key);
	if (conns == NULL) {
		return (NULL);
	}

	/* 从该KEY的连接池中取出一个连接，如果取出为NULL则释放该连接池对象 */

	conn = acl_fifo_pop(&conns->conns);
	if (conn == NULL) {
		/* 先从连接池缓存中删除 */
		acl_htable_delete(cache->cache, conns->key, NULL);
		/* 设置释放空的连接池对象的定时器 */
		set_conn_pool_free_timer(conns);
		return (NULL);
	}

	/* 先取消该流之前设置的回调函数 */
#if 1
	acl_aio_del_read_hook(conn->stream, read_callback, conn);
	acl_aio_del_close_hook(conn->stream, read_close_callback, conn);
	acl_aio_del_timeo_hook(conn->stream, read_timeout_callback, conn);
#else
	acl_aio_clean_hooks(conn->stream);
#endif

	if (conn->free_fn)
		conn->free_fn(conn->stream, conn->ctx);
	/* 取消读监听 */
	acl_aio_disable_read(conn->stream);

	cache->nget++;
	return (conn);
}

ACL_ASTREAM *conn_cache_get_stream(CONN_CACHE *cache, const char *key, void **ctx_pptr)
{
	CONN *conn;
	ACL_ASTREAM *stream;

	conn = conn_cache_get_conn(cache, key);
	if (conn == NULL) {
		if (ctx_pptr)
			*ctx_pptr = NULL;
		return (NULL);
	}

	if (ctx_pptr)
		*ctx_pptr = conn->ctx;
	stream = conn->stream;
	acl_myfree(conn);  /* 因为已经取出流对象，所以可以释放 CONN 对象 */
	return (stream);
}

void conn_cache_delete_key(CONN_CACHE *cache, const char *key)
{
	CONN_POOL *conns;
	ACL_ITER iter;

	conns = (CONN_POOL*) acl_htable_find(cache->cache, key);
	if (conns == NULL)
		return;

	/* 遍历连接池中的连接流并一一关闭 */
	acl_foreach(iter, &conns->conns) {
		CONN *conn = (CONN*) iter.data;
		if (conn->stream) {
			acl_aio_iocp_close(conn->stream);
			conn->stream = NULL;
		}
	}

	/* 将该连接池从连接池缓存中删除 */
	acl_htable_delete(cache->cache, conns->key, NULL);
	/* 设置释放池对象的定时器 */
	set_conn_pool_free_timer(conns);
}

void conn_cache_delete_conn(CONN *conn)
{
	/* 释放并关闭该连接对象 */
	conn_close(conn);
}

void conn_cache_delete_stream(CONN_CACHE *cache, ACL_ASTREAM *stream)
{
	const char *key = ACL_VSTREAM_PEER(acl_aio_vstream(stream));
	CONN_POOL *conns;
	CONN *conn;
	ACL_ITER iter;

	conns = acl_htable_find(cache->cache, key);
	if (conns == NULL)
		return;
	/* 遍历连接池队列中的所有连接 */
	acl_foreach(iter, &conns->conns) {
		conn = (CONN*) iter.data;
		if (conn->stream == stream) {
			/* 释放并关闭该连接对象 */
			conn_close(conn);
			break;
		}
	}
}
