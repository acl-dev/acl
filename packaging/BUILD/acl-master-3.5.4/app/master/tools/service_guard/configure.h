#pragma once

extern char *var_cfg_redis_addrs;
extern char *var_cfg_redis_passwd;
extern char *var_cfg_main_service_list;
extern acl::master_str_tbl var_conf_str_tab[];

extern acl::master_bool_tbl var_conf_bool_tab[];

extern int   var_cfg_threads_max;
extern int   var_cfg_threads_idle;
extern int   var_cfg_redis_conn_timeout;
extern int   var_cfg_redis_rw_timeout;
extern acl::master_int_tbl var_conf_int_tab[];

extern acl::master_int64_tbl var_conf_int64_tab[];

extern acl::redis_client_cluster var_redis;
extern std::map<acl::string, bool> var_main_service_list;
