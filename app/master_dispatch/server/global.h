#pragma once

////////////////////////////////////////////////////////////////////////////////
// ≈‰÷√ƒ⁄»›œÓ

extern char *var_cfg_backend_service;
extern char *var_cfg_status_servers;
extern char *var_cfg_session_addr;
extern acl::master_str_tbl var_conf_str_tab[];

extern acl::master_bool_tbl var_conf_bool_tab[];

extern int   var_cfg_rw_timeout;
extern acl::master_int_tbl var_conf_int_tab[];

extern acl::master_int64_tbl var_conf_int64_tab[];

////////////////////////////////////////////////////////////////////////////////

extern acl::string var_cfg_local_addr;

#define	DEBUG_MIN	100
#define	DEBUG_SVR	(DEBUG_MIN + 1)
