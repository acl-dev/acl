#ifndef	__GLOBAL_INCLUDE_H__
#define	__GLOBAL_INCLUDE_H__

/* 配置文件项 */

extern int   var_cfg_debug_mem;
extern int   var_cfg_loop_enable;
extern int   var_cfg_sync_gid;
extern ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[];

extern int   var_cfg_debug_section;
extern int   var_cfg_gid_step;
extern int   var_cfg_gid_test;
extern int   var_cfg_fh_limit;
extern int   var_cfg_io_timeout;
extern ACL_CONFIG_INT_TABLE service_conf_int_tab[];

extern char *var_cfg_gid_path;
extern char *var_cfg_proto_list;
extern ACL_CONFIG_STR_TABLE service_conf_str_tab[];

/* 服务器之间传输的命令字定义 */
#define	CMD_NEW_GID	"new_gid"
#define CMD_TEST_GID	"test_gid"
	
#endif
