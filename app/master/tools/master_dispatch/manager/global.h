#pragma once

//////////////////////////////////////////////////////////////////////////////

extern char *var_cfg_servers;
extern char *var_cfg_index_page;
extern char *var_cfg_login_page;
extern char *var_cfg_html_charset;
extern char *var_cfg_pop_server;
extern char *var_cfg_allow_users;
extern char *var_cfg_memcache_addr;
extern char *var_cfg_session_key;
extern char *var_cfg_path_info;
extern acl::master_str_tbl var_conf_str_tab[];

extern int   var_cfg_auth_enable;
extern int   var_cfg_pull_data;
extern acl::master_bool_tbl var_conf_bool_tab[];

extern int   var_cfg_conn_timeout;
extern int   var_cfg_rw_timeout;
extern int   var_cfg_session_ttl;
extern int   var_cfg_status_ttl;
extern acl::master_int_tbl var_conf_int_tab[];

extern acl::master_int64_tbl var_conf_int64_tab[];

//////////////////////////////////////////////////////////////////////////////
#define	DEBUG_MIN	100
#define	DEBUG_CONN	(DEBUG_MIN + 1)
