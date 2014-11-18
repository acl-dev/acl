#ifndef	__HTTP_PLUGIN_INCLUDE_H__
#define	__HTTP_PLUGIN_INCLUDE_H__

#include "lib_acl.h"
#include "lib_protocol.h"

#ifdef	__cplusplus
extern "C" {
#endif

/*------------------------- 动态模块需要的函数接口 ---------------------------*/

/**
 * HTTP 初始化注册函数, 该函数如果非空，则由主程序在刚开始运行时调用
 * @param dll_env {ACL_DLL_ENV*} DLL 环境变量, dll_env->logfp 日志文件句柄,
 *  如果不希望将日志记在主程序日志文件中，则可以将此值置空
 * @param dmptr {ACL_DEBUG_MEM*} 用于内存调试的句柄
 * @param cfg_dir {const char*} 配置文件所在路径，该路径下存放所有动态库的配置文件
 */
void http_plugin_init(ACL_DLL_ENV *dll_env, const char *cfg_dir);

/**
 * HTTP 请求过滤注册函数，该函数如果非空，则由主程序在收到HTTP请求头时调用
 * @param client {ACL_VSTREAM*} 客户端流
 * @param hdr_req {HTTP_HDR_REQ*} HTTP请求头
 * @param ctx_ptr {void**} 该指针可以用来存储用户的动态数据
 * @return {int} < 0: 表示禁止该HTTP请求, 该值表示错误号，可以为：-5xx/-4xx;
 *  0: 表示该HTTP请求继续由主程序处理; > 0: 表示该HTTP请求可以由外挂模块的单独
 *  线程处理
 */
int http_request_filter(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void **ctx_ptr);

/**
 * HTTP 请求接管处理注册函数，该函数如果非空，且 http_request_filter() > 0，则由
 * 主程序调用此函数完全接管该HTTP请求及响应，主程序不再处理该HTTP请求及HTTP响应
 * @param client {ACL_VSTREAM*} HTTP客户端数据连接流
 * @param hdr_req {HTTP_HDR_REQ*} 客户端HTTP请求头
 * @param ctx {void*} 过滤器的参数，该动态参数是由 http_request_filter 中的 ctx_ptr
 *  参数返回的
 */
void http_request_forward(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, void *ctx);

/*------------------------------ 其它函数接口 --------------------------------*/

#define	DEBUG_BASE	500
#define	DBG_REQ		(DEBUG_BASE + 1)
#define	DBG_RES		(DEBUG_BASE + 2)

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

extern char *var_cfg_log_name;
extern char *var_cfg_http_domain_allow;
extern char *var_cfg_http_domain_redirect;
extern int   var_cfg_http_domain_allow_all;

#ifdef	__cplusplus
}
#endif

#endif
