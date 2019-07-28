#ifndef __SERVICE_STRUCT_INCLUDE_H__
#define __SERVICE_STRUCT_INCLUDE_H__

#include "lib_acl.h"
#include "conn_cache.h"
#include "dns.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SERVICE SERVICE;
typedef struct CLIENT_ENTRY CLIENT_ENTRY;

/* 动态库加载函数类型定义 */

/* 服务初始化函数类型定义 */
typedef void (*module_service_init_fn)(ACL_DLL_ENV *dll_env, const char *cfgdir);
/* 服务创建函数类型定义 */
typedef SERVICE *(*module_service_create_fn)(void);
/* 服务入口函数类型定义 */
typedef void (*module_service_main_fn)(SERVICE *service, ACL_ASTREAM *stream);

typedef struct {
	module_service_init_fn mod_init;
	module_service_create_fn mod_create;
	module_service_main_fn mod_main;
} MODULE_SERVICE;

struct SERVICE {
	char  name[256];			/* 服务名称 */
	ACL_AIO *aio;				/* 异步IO句柄 */
	acl_pthread_pool_t *wq;			/* 为了兼容老的外挂模块所需要的线程池句柄 */
	ACL_ASTREAM *sstream;			/* 监听套接口 */
	int   conn_timeout;			/* 默认的连接超时时间 */
	int   rw_timeout;			/* 默认的IO超时时间 */
	ACL_DNS *dns_handle;			/* 采用直接发送DNS协议方式查询 */
	DNS_SERVER *dns_server;			/* DNS查询句柄 */
	ACL_HTABLE *dns_table;			/* DNS查询对象哈希表 */
	char  local_addr[256];			/* 监听的本地地址 */
	char **bind_ip_list;			/* 在连接服务器时允许绑定本机的IP地址列表 */
	int   bind_ip_index;			/* 当前需要绑定的IP */

	CONN_CACHE *conn_cache;			/* 长连接连接池对象 */
	MODULE_SERVICE *module;			/* 所属的服务模块 */
};

typedef struct DNS_RING {
	char  domain_key[256];
	ACL_RING ring;
	int   nrefer;
} DNS_RING;

struct CLIENT_ENTRY {
	SERVICE *service;			/* 指向异步代理句柄 */
	ACL_ASTREAM *client;			/* 来自于客户端的连接流 */
	ACL_ASTREAM *server;			/* 连接至服务端的连接流 */

	int   nrefer;				/* 引用计数 */

	ACL_RING dns_entry;			/* DNS查询表的某个链结点 */
	char  domain_key[256];			/* 域名(小写) */
	char  domain_addr[64];			/* 域名所对应的一个IP地址 */

	DNS_CTX dns_ctx;			/* 与服务器域名相关的地址信息 */
	int   ip_idx;				/* 当前所引用的IP地址的索引 */
	int   ip_ntry;				/* 重试 IP 次数 */
	int   server_port;			/* 服务端PORT */
	char  client_ip[32];			/* 客户端IP地址 */
	int   client_port;			/* 服务端PORT */
	const char *dns_errmsg;

	struct {
		time_t begin;			/* 会话开始时间截 */
		time_t stamp;			/* 动态改变的时间截 */

		time_t dns_lookup;		/* DNS查询时间(以秒为单位) */
		time_t connect;			/* 连接服务器的时间(以秒为单位) */
	} tm;

	int   flag_has_replied;

	int   flag_conn_reuse;			/* 重复利用连接池中的连接 */
	int   nretry_on_error;			/* 出错重试次数 */
#define MAX_RETRIED	10

	void (*free_fn)(CLIENT_ENTRY*);
	void (*nslookup_notify_fn)(CLIENT_ENTRY*, int);
#define	NSLOOKUP_OK	0
#define	NSLOOKUP_ERR	-1

	int  (*connect_notify_fn)(CLIENT_ENTRY*);
	void (*connect_timeout_fn)(CLIENT_ENTRY*);
	void (*connect_error_fn)(CLIENT_ENTRY*);
};

#ifdef __cplusplus
}
#endif

#endif
