#pragma once

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

extern char* var_cfg_dbcharset;
extern acl::master_str_tbl var_conf_str_tab[];

extern acl::master_bool_tbl var_conf_bool_tab[];

extern int   var_cfg_conn_timeout;
extern int   var_cfg_rw_timeout;
extern acl::master_int_tbl var_conf_int_tab[];

extern acl::master_int64_tbl var_conf_int64_tab[];


////////////////////////////////////////////////////////////////////////////////
// 其它全局项

extern acl::db_pool* var_dbpool;
extern acl::thread_pool* var_thrpool;
