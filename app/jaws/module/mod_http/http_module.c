#include "lib_acl.h"
#include "service.h"
#include "http_service.h"
#include "http_module.h"

static char *var_cfg_http_plugin_dlnames;
static char *var_cfg_http_plugin_cfgdir;
static char *var_cfg_http_filter_info;
static char *var_cfg_http_tmpl_path;
static char *var_cfg_http_vhost_path;
static char *var_cfg_http_vhost_default;

static ACL_CFG_STR_TABLE __conf_str_tab[] = {
	/* 配置项名称, 配置项缺省值, 存储配置项值的地址 */

	{ "http_filter_proxy", "HTTP_FILTER_PROXY", &var_cfg_http_filter_info },
	{ "http_tmpl_path", "/opt/jaws/www/tmpl", &var_cfg_http_tmpl_path },
	{ "http_vhost_path", "/opt/jaws/conf/www", &var_cfg_http_vhost_path },
	{ "http_vhost_default", "/opt/jaws/conf/default/default.cf", &var_cfg_http_vhost_default },

	{ "http_plugin_dlnames", "", &var_cfg_http_plugin_dlnames },
	{ "http_plugin_cfgdir", "/tmp", &var_cfg_http_plugin_cfgdir},

	{ 0, 0, 0 }
};

static int   var_cfg_http_server_conn_limit;
static int   var_cfg_http_buf_size;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ "http_server_conn_limit", 1000, &var_cfg_http_server_conn_limit, 0, 0 },
	{ "http_buf_size", 8192, &var_cfg_http_buf_size, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static int   var_cfg_http_debug_mem;
static int   var_cfg_http_use_cache;
int   var_cfg_http_client_keepalive;
int   var_cfg_http_server_keepalive;
int   var_cfg_http_method_connect_enable;
int   var_cfg_http_proxy_connection_off;

static ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "debug_mem", 0, &var_cfg_http_debug_mem },
	{ "http_client_keepalive", 1, &var_cfg_http_client_keepalive },
	{ "http_server_keepalive", 1, &var_cfg_http_server_keepalive },
	{ "http_use_cache", 1, &var_cfg_http_use_cache },
	{ "http_method_connect_enable", 0, &var_cfg_http_method_connect_enable },
	{ "http_proxy_connection_off", 0, &var_cfg_http_proxy_connection_off },
	{ 0, 0, 0 },
};

static ACL_DLL_ENV __dll_env;
ACL_DLL_ENV *var_http_dll_env = NULL;

void module_service_init(ACL_DLL_ENV *dll_env, const char *cfg_dir)
{
	const char *myname = "module_service_init";
	ACL_XINETD_CFG_PARSER *cfg;
	char *filepath;

	if (dll_env)
		memcpy(&__dll_env, dll_env, sizeof(ACL_DLL_ENV));
	else
		memset(&__dll_env, 0, sizeof(ACL_DLL_ENV));

	/* 如果 mem_slice 非空则设置内存分配采用切片分配方式 */
	if (__dll_env.mem_slice) {
		acl_mem_slice_set(__dll_env.mem_slice);
		acl_msg_info("%s(%d): set mem slice now", myname, __LINE__);
	}
	var_http_dll_env = &__dll_env;
	if (var_http_dll_env->logfp) {
		acl_msg_open2(var_http_dll_env->logfp, "jaws-http");
		acl_msg_info("%s(%d): logger opened, %s", __FUNCTION__,
			__LINE__, ACL_VSTREAM_PATH(var_http_dll_env->logfp));
		/*
		var_http_dll_env->logfp = acl_log_fp();
		*/
	}


	filepath = acl_concatenate((cfg_dir && *cfg_dir)
			? cfg_dir : "/tmp", "/http.cf", NULL);
	cfg = acl_xinetd_cfg_load(filepath);
	if (cfg == NULL)
		acl_msg_warn("load cfg(%s) error(%s)",
			filepath, acl_last_serror());

	acl_xinetd_params_str_table(cfg, __conf_str_tab);
	acl_xinetd_params_bool_table(cfg, __conf_bool_tab);
	acl_xinetd_params_int_table(cfg, __conf_int_tab);
	acl_xinetd_cfg_free(cfg);
	acl_myfree(filepath);

	/* 是否调试内存的分析状态 */
	if (var_cfg_http_debug_mem == 1) {
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
	} else if (var_cfg_http_debug_mem == 2) {
		var_http_dll_env->mmd = acl_debug_malloc_init(
				var_http_dll_env->mmd, "log.txt");
	} else if (var_cfg_http_debug_mem == 3) {
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
		var_http_dll_env->mmd = acl_debug_malloc_init(
				var_http_dll_env->mmd, "log.txt");
	}

	/* 设置HTTP服务运行模式: 服务器模式还是代理模式 */
	http_filter_set(var_cfg_http_filter_info);
	/* 服务器模式下加载配置 */
	http_conf_load(var_cfg_http_vhost_path, var_cfg_http_vhost_default);
	/* 加载HMTL模板 */
	http_tmpl_load(var_cfg_http_tmpl_path);

	/* 初始化连接池 */
	if (var_cfg_http_server_conn_limit < 10)
		var_cfg_http_server_conn_limit = 10;

	/* 设置HTTP缓冲区大小 */
	if (var_cfg_http_buf_size > 0) {
		http_buf_size_set(var_cfg_http_buf_size);
	}

	/* 加载所有动态插件库并初始化动态库 */
	http_plugin_load_all(dll_env, var_cfg_http_plugin_dlnames, var_cfg_http_plugin_cfgdir);
}

SERVICE *module_service_create()
{
	HTTP_SERVICE *service;

	/* 创建 HTTP 服务对象 */
	service = http_service_new();
	return ((SERVICE*) service);
}


void module_service_main(SERVICE *service, ACL_ASTREAM *stream)
{
	http_service_main((HTTP_SERVICE*) service, stream);
}
