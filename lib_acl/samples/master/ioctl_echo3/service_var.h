#ifndef	__SERVICE_VAR_INCLUDE_H__
#define	__SERVICE_VAR_INCLUDE_H__

#include "lib_acl.h"

/*------------- 字符串配置项 ----------------*/

extern ACL_CFG_STR_TABLE var_conf_str_tab[];

/* 日志调试输出信息 */
extern char *var_cfg_debug_msg;

/*-------------- 布尔值配置项 ---------------*/

extern ACL_CFG_BOOL_TABLE var_conf_bool_tab[];

/* 是否输出日志调试信息 */
extern int var_cfg_debug_enable;

/* 是否与客户端保持长连接 */
extern int var_cfg_keep_alive;

/*-------------- 整数配置项 -----------------*/

extern ACL_CFG_INT_TABLE var_conf_int_tab[];

/* 每次与客户端通信时，读超时时间(秒) */
extern int  var_cfg_io_timeout;

#endif
