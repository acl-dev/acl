
#ifndef	__SERVICE_MAIN_INCLUDE_H__
#define	__SERVICE_MAIN_INCLUDE_H__

#include "lib_acl.h"

#ifdef	__cplusplus
extern "C" {
#endif

/* 配置文件项 */
/* in service_main.c */

extern int   var_cfg_debug_mem;
extern char *var_cfg_dbpath;
extern char *var_cfg_dbnames;
extern int   var_cfg_rw_timeout;
extern int   var_cfg_use_bdb;

extern ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[];
extern ACL_CONFIG_INT_TABLE service_conf_int_tab[];
extern ACL_CONFIG_STR_TABLE service_conf_str_tab[];

/**
 * 初始化函数，服务器模板框架启动后仅调用该函数一次
 * @param init_ctx {void*} 用户自定义类型指针, 可以在调用
 *  app_main() 时引入
 */
extern void service_init(void *init_ctx);

/**
 * 进程退出时的回调函数
 * @param exist_ctx {void*} 用户自定义类型指针
 */
extern void service_exit(void *exit_ctx);

/**
 * 协议处理函数入口
 * @param stream {ACL_VSTREAM*} 客户端数据连接流
 * @param run_ctx {void*} 用户自定义类型指针
 */
extern int service_main(ACL_ASTREAM *astream, void *run_ctx);

#ifdef	__cplusplus
}
#endif

#endif
