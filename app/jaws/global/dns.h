
#ifndef __DNS_SERVER_INCLUDE_H__
#define __DNS_SERVER_INCLUDE_H__

#include "lib_acl.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USE_THREAD_POOL			/* 外挂查询DNS模块采用线程池方式 */

/* 查询DNS的消息类型定义 */
#define DNS_MSG_LOOKUP		ACL_MSGIO_USER + 100
#define DNS_MSG_LOOKUP_RESULT	ACL_MSGIO_USER + 101

#define DNS_IP_MAX		5

typedef struct DNS_CTX DNS_CTX;
typedef struct DNS_SERVER DNS_SERVER;
typedef struct DNS_CACHE DNS_CACHE;

typedef void (*DNS_CALLBACK)(const DNS_CTX *);

struct DNS_CTX {
	/* public */
	char  domain_key[256];		/* 域名(小写) */
	char  ip[DNS_IP_MAX][64];	/* IP地址数组 */
	int   port[DNS_IP_MAX];		/* port 数组 */
	int   ip_cnt;			/* IP地址数组中的有效IP地址个数 */
	time_t begin;			/* 开始查询时的时间截 */

	/* public */
	DNS_CALLBACK callback;		/* 查询返回调用的用户的回调函数 */
	void *context;			/* 用户传递的参数 */

	/* private */
	DNS_SERVER *dns;		/* DNS对象句柄，内部变量 */
	ACL_MSGIO  *mio;		/* IO消息句柄，内部变量 */
	ACL_DNS_DB *dns_db;		/* 临时用以传递DNS查询结果，内部变量 */
};

struct DNS_SERVER {
	ACL_AIO *aio;			/* 异步通信框架 */
	acl_pthread_pool_t *wq;		/* 线程池句柄 */
	ACL_MSGIO *listener;		/* 消息监听者句柄 */
	ACL_MSGIO *mio;			/* 消息句柄 */
	char  addr[256];

	DNS_CACHE *dns_cache;		/* DNS缓存句柄 */
	acl_pthread_mutex_t lock;	/* 线程锁 */
};

/* in dns_server.c */
DNS_SERVER *dns_server_create(ACL_AIO *aio, int timeout);
void dns_server_close(DNS_SERVER *dns);
void dns_server_static_add(DNS_SERVER *dns, const char *map, const char *delim, int def_port);
int dns_server_lookup(DNS_SERVER *dns, const DNS_CTX *ctx);

/* in dns_cache.c */
void dns_cache_push_one(DNS_CACHE *dns_cache, const ACL_DNS_DB *dns_db, int timeout);
void dns_cache_push(DNS_CACHE *dns_cache, const ACL_DNS_DB *dns_db);
void dns_cache_push2(DNS_CACHE *dns_cache, const DNS_CTX *dns_ctx);

ACL_DNS_DB *dns_cache_lookup(DNS_CACHE *dns_cache, const char *name);
void dns_cache_del_host(DNS_CACHE *dns_cache, const char *name);
DNS_CACHE *dns_cache_create(int timeout, int thread_safe);

/* in dns_hosts.c */
void dns_hosts_load(ACL_DNS *dns_handle, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
