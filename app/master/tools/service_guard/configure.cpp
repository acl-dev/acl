#include "stdafx.h"
#include "configure.h"

//////////////////////////////////////////////////////////////////////////////
// 配置内容项

char *var_cfg_redis_addrs;
char *var_cfg_redis_passwd;
char *var_cfg_main_service_list;
acl::master_str_tbl var_conf_str_tab[] = {
	{ "redis_addrs", "127.0.0.1:6379", &var_cfg_redis_addrs },
	{ "redis_passwd", "", &var_cfg_redis_passwd },
	{ "main_service_list", "", &var_cfg_main_service_list },

	{ 0, 0, 0 }
};

acl::master_bool_tbl var_conf_bool_tab[] = {

	{ 0, 0, 0 }
};

int   var_cfg_threads_max;
int   var_cfg_threads_idle;
int   var_cfg_redis_conn_timeout;
int   var_cfg_redis_rw_timeout;
acl::master_int_tbl var_conf_int_tab[] = {
	{ "threads_max", 256, &var_cfg_threads_max, 0, 0 },
	{ "threads_idle", 60, &var_cfg_threads_idle, 0, 0 },
	{ "redis_conn_timeout", 10, &var_cfg_redis_conn_timeout, 0, 0 },
	{ "redis_rw_timeout", 10, &var_cfg_redis_rw_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

acl::master_int64_tbl var_conf_int64_tab[] = {

	{ 0, 0 , 0 , 0, 0 }
};

acl::redis_client_cluster var_redis;
std::map<acl::string, bool> var_main_service_list;
