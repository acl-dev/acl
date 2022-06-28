#include "lib_acl.h"
#include "http_plugin.h"

char *var_cfg_log_name;
char *var_cfg_cgi_bin;

static ACL_CFG_STR_TABLE __conf_str_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "logpath", "/tmp/cgifilter.log", &var_cfg_log_name },
	{ "cgi-bin", "/opt/jaws/www/cgi-bin", &var_cfg_cgi_bin },

	{ 0, 0, 0 }
};

int   var_cfg_data_clone;
static int   var_cfg_debug_mem;

static ACL_CFG_BOOL_TABLE __conf_bool_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "data_clone", 0, &var_cfg_data_clone },
	{ "debug_mem", 0, &var_cfg_debug_mem },
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
	const ACL_ARRAY *cgi_maps;

	/* 读取配置文件 */

	filepath = acl_concatenate((cfg_dir && *cfg_dir)
			? cfg_dir : "/tmp", "/cgi.cf", NULL);
	cfg = acl_xinetd_cfg_load(filepath);
	if (cfg == NULL)
		acl_msg_warn("load cfg(%s) error(%s)",
			filepath, acl_last_serror());

	acl_xinetd_params_str_table(cfg, __conf_str_tab);
	acl_xinetd_params_int_table(cfg, __conf_int_tab);
	acl_xinetd_params_bool_table(cfg, __conf_bool_tab);

	cgi_maps = acl_xinetd_cfg_get_ex(cfg, "cgi-map");
	if (cgi_maps) {
		ACL_ITER iter;

		acl_foreach(iter, (ACL_ARRAY*) cgi_maps) {
			const char *cgi_map = (const char*) iter.data;
			ACL_ARGV *cgi_argv = acl_argv_split(cgi_map, "\t ");
			ACL_ITER iter2;

			if (cgi_argv->argc < 2) {
				acl_msg_warn("%s(%d): cgi-map(%s) invalid",
					__FUNCTION__, __LINE__, cgi_map);
				acl_argv_free(cgi_argv);
				continue;
			}
			acl_foreach(iter2, cgi_argv) {
				char *ptr = (char*) iter2.data;
				acl_lowercase(ptr);
			}
			http_cgi_add1(cgi_argv);
			acl_argv_free(cgi_argv);
		}
	}

	if (cfg)
		acl_xinetd_cfg_free(cfg);
	acl_myfree(filepath);

	/* 初始化CGI模块 */
	http_cgi_init();

	/* 创建线程池 */
	http_plugin_pool_create(var_cfg_threads_limit, var_cfg_threads_idle);

	http_plugin_debug_memory(var_cfg_debug_mem);
}
