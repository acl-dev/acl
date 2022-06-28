#ifndef	__SERVICE_CONF_INCLUDE_H__
#define	__SERVICE_CONF_INCLUDE_H__

/* ≈‰÷√Œƒº˛œÓ */
/* in service_main.c */

extern int   var_cfg_debug_mem;

extern ACL_CONFIG_BOOL_TABLE service_conf_bool_tab[];

extern int   var_cfg_nthreads;
extern int   var_cfg_dns_lookup_timeout;
extern int   var_cfg_dns_cache_limit;
extern int   var_cfg_aio_buf_size;
extern int   var_cfg_server_conn_limit;

extern ACL_CONFIG_INT_TABLE service_conf_int_tab[];

extern char *var_cfg_dns_list;
extern char *var_cfg_hosts_list;
extern char *var_cfg_proto_name;
extern char *var_cfg_service_dlnames;
extern char *var_cfg_service_cfgdir;
extern char *var_cfg_bind_ip_list;

extern ACL_CONFIG_STR_TABLE service_conf_str_tab[];

#endif
