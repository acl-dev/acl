#ifndef __HTTP_SERVICE_INCLUDE_H__
#define __HTTP_SERVICE_INCLUDE_H__

#include "lib_acl.h"
#include <time.h>
#include "lib_protocol.h"

#include "service_struct.h"
#include "file_cache.h"
#include "http_vhost.h"
#include "dns.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DEBUG_PROXY_BASE	60
#define DEBUG_HTML_TITLE	(DEBUG_PROXY_BASE + 1)

/* 常量定义 */
#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN
#define	SCP	ACL_SAFE_STRNCPY

#define HTTP_HDRLEN_DEF		8192

#define	HTTP_FILTER_ERROR	0
#define	HTTP_FILTER_HTTPD	(1<<0)
#define	HTTP_FILTER_PROXY	(1<<1)

typedef struct HTTP_SERVICE HTTP_SERVICE;
typedef struct HTTP_CLIENT HTTP_CLIENT;

/* 过滤器初始化函数类型定义 */
typedef void (*plugin_init_fn)(ACL_DLL_ENV *dll_env, const char *plugin_cfg_dir);

/* 过滤器匹配函数类型定义 */
typedef int (*plugin_filter_request_fn)(ACL_VSTREAM *client,
					HTTP_HDR_REQ *hdr_req,
					void **ctx_ptr);
typedef int (*plugin_filter_respond_fn)(ACL_VSTREAM *client,
					ACL_VSTREAM *server,
					HTTP_HDR_REQ *hdr_req,
					HTTP_HDR_RES *hdr_res,
					void **ctx_ptr);

/* 过滤器接管函数类型定义 */
typedef void (*plugin_forward_request_fn)(ACL_VSTREAM *client,
		HTTP_HDR_REQ *hdr_req, void *ctx);
typedef void (*plugin_forward_respond_fn)(ACL_VSTREAM *client, ACL_VSTREAM *server,
		HTTP_HDR_REQ *hdr_req, HTTP_HDR_RES *hdr_res, void *ctx);

/* HTTP数据体过滤器函数类型定义 */
typedef char *(*plugin_dat_filter_fn)(const char *data, int dlen,
		int *ret, int *stop, void *ctx);
/* 释放HTTP数据体过滤器分配的内存 */
typedef void (*plugin_dat_free_fn)(void *buf, void *ctx);

/* 过滤器结构类型定义 */
typedef struct HTTP_PLUGIN {
	plugin_init_fn init;
	union {
		plugin_filter_request_fn request;	/* 请求过滤器 */
		plugin_filter_respond_fn respond;	/* 响应过滤器 */
	} filter;	/* 过滤器接口 */
	union {
		plugin_forward_request_fn request;	/* 请求接管器 */
		plugin_forward_respond_fn respond;	/* 响应接管器 */
	} forward;	/* 接管器接口 */
	plugin_dat_filter_fn data_filter;		/* 数据体过滤器 */
	plugin_dat_free_fn data_free;			/* 释放动态内存 */
} HTTP_PLUGIN;

struct HTTP_SERVICE {
	SERVICE service;
	ACL_VSTRING *file_path;

	ACL_FIFO request_plugins;		/* 外挂请求过滤器(HTTP_PLUGIN)集合 */
	ACL_FIFO respond_plugins;		/* 外挂响应过滤器(HTTP_PLUGIN)集合 */
	ACL_FIFO request_dat_plugins;		/* 外挂请求过滤器(HTTP_PLUGIN)集合 */
	ACL_FIFO respond_dat_plugins;		/* 外挂响应过滤器(HTTP_PLUGIN)集合 */
};

typedef struct {
	HTTP_CLIENT  *http_client;
	HTTP_HDR_REQ *hdr_req;			/* HTTP协议请求头指针 */
	HTTP_REQ     *req;			/* HTTP协议请求 */
	int   flag;
#define	CLIENT_READ_WAIT		(1 << 2)
#define	SERVER_READ_WAIT		(1 << 3)
} HTTP_CLIENT_REQ;

struct HTTP_CLIENT {
	CLIENT_ENTRY  entry;			/* 基本代理对象 */
	HTTP_CLIENT_REQ *req_curr;		/* 当前正被处理的请求 */
	ACL_FIFO      req_list;			/* HTTP请求队列 */
#define	WRITE_TO_CLIENT(x, buff, dlen)	acl_aio_writen((x)->entry.client, (buff), (int) (dlen))
#define	WRITEV_TO_CLIENT(x, vect, cnt)	acl_aio_writev((x)->entry.client, (vect), (int) (cnt))
#define	WRITE_TO_SERVER(x, buff, dlen)	acl_aio_writen((x)->entry.server, (buff), (int) (dlen))
	HTTP_HDR_RES *hdr_res;			/* HTTP协议响应头指针 */
	HTTP_RES     *res;			/* HTTP协议响应 */
	ACL_VSTREAM  *fp;

	int   use_cache;
	ACL_VSTRING  *buf;			/* 内部用的动态内存 */
	FILE_CACHE   *cache;
	CACHE_ITER    cache_iter;

	unsigned int  flag;
#define	HTTP_FLAG_CLIENT_LOCKED		(1 << 0)
#define	HTTP_FLAG_SERVER_LOCKED		(1 << 1)
#define	HTTP_FLAG_CLIENT_CLOSED		(1 << 3)
#define	HTTP_FLAG_SERVER_CLOSED		(1 << 4)
#define	HTTP_FLAG_REQEND		(1 << 5)
#define	HTTP_FLAG_FINISH		(1 << 6)
#define	HTTP_FLAG_CLIENT_KEEP_ALIVE	(1 << 7)
#define	HTTP_FLAG_SERVER_KEEP_ALIVE	(1 << 8)

	size_t total_size;
	size_t sent_size;

	struct {
		time_t read_reqhdr;		/* 读HTTP协议请求头时间 */
		time_t read_reqbody;		/* 读HTTP协议请求体时间 */
		time_t read_reshdr;		/* 读HTTP协议响应头时间 */
		time_t read_resbody;		/* 读HTTP协议响应体时间 */
		time_t send_reqhdr;		/* 写HTTP协议请求头时间 */
		time_t send_reqbody;		/* 写HTTP协议请求体时间 */
		time_t send_reshdr;		/* 写HTTP协议响应头时间 */
		time_t send_resbody;		/* 写HTTP协议响应体时间 */
	} tm;

	plugin_dat_filter_fn request_filter;	/* 外挂模块HTTP请求数据体过滤器 */
	plugin_dat_filter_fn respond_filter;	/* 外挂模块HTTP响应数据体过滤器 */
	void *plugin_req_ctx;			/* 外挂模块在HTTP请求阶段的动态参数 */
	void *plugin_res_ctx;			/* 外挂模块在HTTP响应阶段的动态参数 */
};

/* in html_template.c */
extern char HTTP_REPLY_DNS_ERR[];
extern char HTTP_REPLY_TIMEOUT[];
extern char HTTP_REPLY_ERROR[];
extern char HTTP_SEND_ERROR[];
extern char HTTP_REQUEST_INVALID[];
extern char HTTP_REQUEST_LOOP[];
extern char HTTP_CONNECT_ERROR[];
extern char HTTP_CONNECT_TIMEOUT[];
extern char HTTP_REQUEST_DENY[];
extern char HTTP_REQUEST_NOFOUND[];
extern char HTTP_INTERNAL_ERROR[];

/* in http_service.c */
void http_service_main(HTTP_SERVICE *service, ACL_ASTREAM *stream);
void http_service_start(HTTP_CLIENT *client);
void http_service_free(HTTP_SERVICE *service);
HTTP_SERVICE *http_service_new(void);

/* in http_filter.c */
int http_filter_type(void);
void http_filter_set(const char *filter_info);

/* in http_plugin.c */
void http_plugin_load(ACL_DLL_ENV *dll_env, const char *dlname, const char *plugin_cfgdir);
void http_plugin_load_all(ACL_DLL_ENV *dll_env, const char *dlnames, const char *plugin_cfgdir);
void http_plugin_unload_all(void);
void http_plugin_set_callback(HTTP_SERVICE *service);

/* in http_conf.c */
void http_conf_load(const char *path, const char *default_cf);

/* in http_client.c */
HTTP_CLIENT *http_client_new(HTTP_SERVICE *service, ACL_ASTREAM *stream);
void http_client_free(CLIENT_ENTRY *entry);
void http_client_reset(HTTP_CLIENT *client);
HTTP_CLIENT_REQ *http_client_req_new(HTTP_CLIENT *http_client);
void http_client_req_free(HTTP_CLIENT_REQ *req);
int http_client_req_filter(HTTP_CLIENT *http_client);

/* in http_server.c */
int http_server_start(HTTP_CLIENT *http_client);

/* in http_proxy.c */
int http_proxy_start(HTTP_CLIENT *entry);

/* in tcp_proxy.c */
void tcp_start(CLIENT_ENTRY *entry);

#ifdef __cplusplus
}
#endif

#endif
