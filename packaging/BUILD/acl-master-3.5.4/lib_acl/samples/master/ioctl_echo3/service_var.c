#include "lib_acl.h"
#include "service_var.h"

char *var_cfg_debug_msg;

ACL_CFG_STR_TABLE var_conf_str_tab[] = {
	{ "debug_msg", "test_msg", &var_cfg_debug_msg },

	{ 0, 0, 0 }
};

int  var_cfg_debug_enable;
int  var_cfg_keep_alive;

ACL_CFG_BOOL_TABLE var_conf_bool_tab[] = {
	{ "debug_enable", 1, &var_cfg_debug_enable },
	{ "keep_alive", 1, &var_cfg_keep_alive },

	{ 0, 0, 0 }
};

int  var_cfg_io_timeout;

ACL_CFG_INT_TABLE var_conf_int_tab[] = {
	{ "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },

	{ 0, 0 , 0 , 0, 0 }
};

