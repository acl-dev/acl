#pragma once

////////////////////////////////////////////////////////////////////////////////
// 配置内容项

extern char *var_cfg_rpc_addr;
extern acl::master_str_tbl var_conf_str_tab[];

extern int   var_cfg_preread;
extern acl::master_bool_tbl var_conf_bool_tab[];

extern int   var_cfg_nthreads_limit;
extern int   var_cfg_echo_length;
extern acl::master_int_tbl var_conf_int_tab[];

////////////////////////////////////////////////////////////////////////////////

extern bool  var_mem_slice_on;
