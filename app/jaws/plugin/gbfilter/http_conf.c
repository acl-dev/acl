#include "lib_acl.h"
#include "http_plugin.h"

char *var_cfg_log_name;

static ACL_CFG_STR_TABLE __conf_str_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "logpath", "/tmp/gbfilter.log", &var_cfg_log_name },
	{ 0, 0, 0 }
};

int   var_cfg_data_clone;
int   var_cfg_rewrite_enable;
static int   var_cfg_debug_mem;

static ACL_CFG_BOOL_TABLE __conf_bool_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "data_clone", 0, &var_cfg_data_clone },
	{ "debug_mem", 0, &var_cfg_debug_mem },
	{ "rewrite_enable", 1, &var_cfg_rewrite_enable },
	{ 0, 0 , 0 }
};

static int var_cfg_threads_limit;
static int var_cfg_threads_idle;

static ACL_CFG_INT_TABLE __conf_int_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "threads_limit", 100, &var_cfg_threads_limit, 0, 0 },
	{ "threads_idle", 120, &var_cfg_threads_idle, 0, 0 },
	{ 0, 0 , 0, 0, 0 }
};

void http_conf_load(const char *cfg_dir)
{
	char *filepath;
	ACL_XINETD_CFG_PARSER *cfg;

	/* 读取配置文件 */

	filepath = acl_concatenate((cfg_dir && *cfg_dir)
			? cfg_dir : "/tmp", "/http_gb.cf", NULL);
	cfg = acl_xinetd_cfg_load(filepath);
	if (cfg == NULL)
		acl_msg_warn("load cfg(%s) error(%s)",
			filepath, acl_last_serror());

	acl_xinetd_params_str_table(cfg, __conf_str_tab);
	acl_xinetd_params_int_table(cfg, __conf_int_tab);
	acl_xinetd_params_bool_table(cfg, __conf_bool_tab);

	if (cfg)
		acl_xinetd_cfg_free(cfg);
	acl_myfree(filepath);

	/* 创建线程池 */

	http_plugin_pool_create(var_cfg_threads_limit, var_cfg_threads_idle);
}
