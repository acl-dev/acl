#include "lib_acl.h"
#include "service_conf.h"

int   var_cfg_debug_mem;

ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "debug_mem", 0, &var_cfg_debug_mem },
	{ 0, 0, 0 }
};

int   var_cfg_nthreads;
int   var_cfg_dns_lookup_timeout;
int   var_cfg_dns_cache_limit;
int   var_cfg_aio_buf_size;
int   var_cfg_server_conn_limit;

ACL_CONFIG_INT_TABLE service_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */
	/* example: { "client_idle_limit", 60, &var_cfg_client_idle_limit, 0, 0 }, */

	{ "nthreads", 2, &var_cfg_nthreads, 0, 0 },
	{ "dns_lookup_timeout", 5, &var_cfg_dns_lookup_timeout, 0, 0 },
	{ "dns_cache_limit", 10000, &var_cfg_dns_cache_limit, 0 , 0 },
	{ "aio_buf_size", 8192, &var_cfg_aio_buf_size, 0, 0 },
	{ "server_conn_limit", 1024, &var_cfg_server_conn_limit, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};

char *var_cfg_dns_list;
char *var_cfg_hosts_list;
char *var_cfg_proto_name;
char *var_cfg_service_dlnames;
char *var_cfg_service_cfgdir;
char *var_cfg_bind_ip_list;

ACL_CFG_STR_TABLE service_conf_str_tab[] = {
	/* TODO: you can add configure variables of (char *) type here */
	/* example: { "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr }, */

	{ "dns_list", "", &var_cfg_dns_list },
	{ "hosts_list", "", &var_cfg_hosts_list },
	{ "proto_name", "http", &var_cfg_proto_name },
	{ "service_dlnames", "/opt/jaws/module", &var_cfg_service_dlnames },
	{ "service_cfgdir", "/opt/jaws/conf/module", &var_cfg_service_cfgdir },
	{ "bind_ip_list", "", &var_cfg_bind_ip_list },
	{ 0, 0, 0 }
};
