#include "lib_acl.h"
#include "http_access.h"
#include "http_redirect.h"
#include "http_plugin.h"

char *var_cfg_log_name;
char *var_cfg_http_domain_allow;
char *var_cfg_http_domain_redirect;

static ACL_CFG_STR_TABLE __conf_str_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "logpath", "/tmp/access_filter.log", &var_cfg_log_name },
	{ "http_domain_allow", "", &var_cfg_http_domain_allow },
	{ "http_domain_redirect", "", &var_cfg_http_domain_redirect },
	{ 0, 0, 0 }
};

static int   var_cfg_debug_mem;
int   var_cfg_http_domain_allow_all;

static ACL_CFG_BOOL_TABLE __conf_bool_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */
	{ "debug_mem", 0, &var_cfg_debug_mem },
	{ "http_domain_allow_all", 1, &var_cfg_http_domain_allow_all },
	{ 0, 0 , 0 }
};

static ACL_DLL_ENV __dll_env;

void http_plugin_init(ACL_DLL_ENV *dll_env, const char *cfg_dir)
{
	ACL_XINETD_CFG_PARSER *cfg;
	char *filepath;

	if (dll_env)
		memcpy(&__dll_env, dll_env, sizeof(ACL_DLL_ENV));
	else
		memset(&__dll_env, 0, sizeof(ACL_DLL_ENV));

        if (__dll_env.logfp) {
                acl_msg_open2(__dll_env.logfp, "http-access");
                acl_msg_info("%s(%d): logger opened, %s", __FUNCTION__,
                        __LINE__, ACL_VSTREAM_PATH(__dll_env.logfp));
	}

	/* 如果 mem_slice 非空则设置内存分配采用切片分配方式 */
	if (__dll_env.mem_slice)
		acl_mem_slice_set(__dll_env.mem_slice);

	filepath = acl_concatenate((cfg_dir && *cfg_dir)
			? cfg_dir : "/tmp", "/http_access.cf", NULL);
	cfg = acl_xinetd_cfg_load(filepath);
	if (cfg == NULL)
		acl_msg_warn("load cfg(%s) error(%s)",
			filepath, acl_last_serror());

	acl_xinetd_params_str_table(cfg, __conf_str_tab);
	acl_xinetd_params_bool_table(cfg, __conf_bool_tab);

	if (cfg)
		acl_xinetd_cfg_free(cfg);
	acl_myfree(filepath);

	/* 针对代理模式，初始化访问列表 */
	http_access_init();

	/* 初始化有关重定向列表 */
	http_redirect_init();

	/* 是否调试插件的内存分配情况 */
	if (var_cfg_debug_mem == 1) {
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
	} else if (var_cfg_debug_mem == 2) {
		__dll_env.mmd = acl_debug_malloc_init(__dll_env.mmd, "access_log.txt");
	} else if (var_cfg_debug_mem == 3) {
		__dll_env.mmd = acl_debug_malloc_init(__dll_env.mmd, "access_log.txt");
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
	}
}
