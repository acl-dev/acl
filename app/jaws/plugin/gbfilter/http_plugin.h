#ifndef	__HTTP_PLUGIN_INCLUDE_H__
#define	__HTTP_PLUGIN_INCLUDE_H__

#include "lib_acl.h"
#include "lib_protocol.h"

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _USRDLL
# ifdef GBFILTER_EXPORTS
#  define HTTP_PLUGIN_API __declspec(dllexport)
# else
#  define HTTP_PLUGIN_API __declspec(dllimport)
# endif
#else
#  define HTTP_PLUGIN_API
#endif

/**
 * HTTP 初始化注册函数, 该函数如果非空，则由主程序在刚开始运行时调用
 * @param dll_env {ACL_DLL_ENV*} DLL 环境变量, dll_env->logfp 日志文件句柄,
 *  如果不希望将日志记在主程序日志文件中，则可以将此值置空
 * @param cfg_dir {const char*} 配置文件所在路径，该路径下存放所有动态库的配置文件
 */
HTTP_PLUGIN_API void http_plugin_init(ACL_DLL_ENV *dll_env, const char *cfg_dir);

/**
 * HTTP 请求过滤注册函数，该函数如果非空，则由主程序在收到HTTP请求头时调用
 * @param client {ACL_VSTREAM*} 客户端流
 * @param hdr_req {HTTP_HDR_REQ*} HTTP请求头
 * @param ctx_ptr {void**} 该指针可以用来存储用户的动态数据
 * @return {int} < 0: 表示禁止该HTTP请求, 该值表示错误号，可以为：-5xx/-4xx;
 *  0: 表示该HTTP请求继续由主程序处理; > 0: 表示该HTTP请求可以由外挂模块的单独
 *  线程处理
 */
HTTP_PLUGIN_API int http_request_filter(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void **ctx_ptr);

/**
 * HTTP 请求接管处理注册函数，该函数如果非空，且 http_request_filter() > 0，则由
 * 主程序调用此函数完全接管该HTTP请求及响应，主程序不再处理该HTTP请求及HTTP响应
 * @param client {ACL_VSTREAM*} HTTP客户端数据连接流
 * @param hdr_req {HTTP_HDR_REQ*} 客户端HTTP请求头
 * @param ctx {void*} 过滤器的参数，该动态参数是由 http_request_filter 中的 ctx_ptr
 *  参数返回的
 */
HTTP_PLUGIN_API void http_request_forward(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void *ctx);

/**
 * HTTP 响应过滤注册函数，该函数如果非空，则由主程序在收到HTTP响应头时调用
 * @param client {ACL_VSTREAM*} HTTP客户端数据连接流
 * @param server {ACL_VSTREAM*} HTTP服务端数据连接流
 * @param hdr_req {HTTP_HDR_REQ*} HTTP请求头
 * @param hdr_res {HTTP_HDR_RES*} HTTP响应头
 * @param ctx_ptr {void**} 该指针可以用来存储用户的动态数据
 * @return {int} 0: 表示该HTTP响应继续由主程序处理; !0: 表示该HTTP响应可以由外
 *  挂模块的单独线程处理
 */
HTTP_PLUGIN_API int http_respond_filter(ACL_VSTREAM *client, ACL_VSTREAM *server,
	HTTP_HDR_REQ *hdr_req, HTTP_HDR_RES *hdr_res, void **ctx_ptr);

/**
 * HTTP 响应接管处理注册函数，该函数如果非空，且 http_respond_filter() > 0, 则由
 * 主程序调用此函数完全接管该HTTP响应处理过程，主程序不再处理该HTTP请求及HTTP响应
 * @param client {ACL_VSTREAM*} HTTP客户端数据连接流
 * @param server {ACL_VSTREAM*} HTTP服务端数据连接流
 * @param hdr_req {HTTP_HDR_REQ*} 客户端HTTP请求头
 * @param hdr_res {HTTP_HDR_RES*} 服务端HTTP响应头
 * @param ctx {void*} 过滤器的参数，该动态参数是由 http_respond_filter 中的 ctx_ptr
 *  参数返回的
 */
HTTP_PLUGIN_API void http_respond_forward(ACL_VSTREAM *client, ACL_VSTREAM *server,
	HTTP_HDR_REQ *hdr_req, HTTP_HDR_RES *hdr_res, void *ctx);

/**
 * HTTP 响应数据体过滤注册函数，可以将源数据转换为目标数据
 * @param data {const char*} 源数据
 * @param dlen {int} data 长度
 * @param ret {int*} 返回新数据的长度, < 0 表示出错
 * @param stop {int*} 禁止下一个过滤器处理过程
 * @param ctx {void*} 过滤器的参数，该动态参数是由 http_respond_filter 中的 ctx_ptr
 *  参数返回的
 * @return {char*} 新数据, 若为空则表示当前数据暂不可用
 */
HTTP_PLUGIN_API char *http_respond_dat_filter(const char *data, int dlen,
                int *ret, int *stop, void *ctx);

/**
 * 释放由 http_respond_dat_filter 产生的动态数据的注册函数
 * @param buf {void*} 新的动态数据地址
 * @param ctx {void*} 过滤器的参数，该动态参数是由 http_respond_filter 中的 ctx_ptr
 *  参数返回的
 */
HTTP_PLUGIN_API void http_respond_dat_free(void *buf, void *ctx);

/*---------------------------------------------------------------------------*/

#define	DEBUG_BASE	500
#define	DBG_REQ		(DEBUG_BASE + 1)
#define	DBG_RES		(DEBUG_BASE + 2)

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

/* 全局配置变量 */

extern char *var_cfg_log_name;
extern int   var_cfg_data_clone;
extern int   var_cfg_rewrite_enable;

/* in http_conf.c */

/**
 * 加载配置文件
 * @param cfg_dir {const char*} 配置文件所在的目录位置
 */
void http_conf_load(const char *cfg_dir);

/* in http_plugin.c */

/**
 * 创建半难留线程池
 * @param threads_limit {int} 线程池中最大线程个数
 * @param threads_idle {int} 线程程中每个线程的最大空闲时间(秒)
 */
void http_plugin_pool_create(int threads_limit, int threads_idle);

/**
 * 向线程池中添加任务，用户添加的任务由线程池中的某个线程接管
 * @param start_routine {void (*)(void*)} 用户添加的任务回调函数
 * @param arg {void*} 线程池中的某个线程回调 start_routine 时的回调参数
 */
void http_plugin_pool_append(void (*start_routine)(void *), void *arg);

/**
 * 当需要调试内存分配状态时的函数
 * @param level {int} 调试级别，可用的级别为: 1, 2, 3
 */
void http_plugin_debug_memory(int level);

#ifdef	__cplusplus
}
#endif

#endif
