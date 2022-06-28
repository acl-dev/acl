#ifndef	__HTTP_MODULE_INCLUDE_H__
#define	__HTTP_MODULE_INCLUDE_H__

#include "lib_acl.h"
#include "service_struct.h"

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef _USRDLL
# ifdef MOD_HTTP_EXPORTS
#  define MOD_HTTP_API __declspec(dllexport)
# else
#  define MOD_HTTP_API __declspec(dllimport)
# endif
#else
#  define MOD_HTTP_API
#endif

extern ACL_DLL_ENV *var_http_dll_env;
extern char *var_cfg_http_domain_allow;
extern int   var_cfg_http_client_keepalive;
extern int   var_cfg_http_server_keepalive;
extern int   var_cfg_http_domain_allow_all;
extern int   var_cfg_http_method_connect_enable;
extern int   var_cfg_http_proxy_connection_off;

/* 动态加载的函数接口 */

/**
 * 被动态加载的模块的初始化函数，仅被调用一次
 * @param dll_env {ACL_DLL_ENV*} 由加载程序传递过来的环境变量
 * @param cfg_dir {const char*} 该动态模块的配置文件所在路径
 */
MOD_HTTP_API void module_service_init(ACL_DLL_ENV *dll_env, const char *cfg_dir);

/**
 * 由动态模块创建一个服务实例，可以被加载程序调用多次以产生多个服务实例
 * @return {SERVICE*} 由动态模块创建的服务对象实例
 */
MOD_HTTP_API SERVICE *module_service_create(void);

/**
 * 当加载程序接收到一个客户端连接后调用此函数
 * @param service {SERVICE*} 由 module_service_create 创建的服务对象
 * @param stream {ACL_ASTREAM*} 由加载程序接收的客户端异步流对象
 */
MOD_HTTP_API void module_service_main(SERVICE *service, ACL_ASTREAM *stream);

#ifdef	__cplusplus
}
#endif

#endif
