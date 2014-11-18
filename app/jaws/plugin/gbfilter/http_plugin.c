#include "lib_acl.h"
#include "http_plugin.h"

static ACL_DLL_ENV __dll_env;
static acl_pthread_pool_t *__thrpool = NULL;

/* 初始化该插件的环境 */

static void plugin_init(ACL_DLL_ENV *dll_env)
{
	if (dll_env)
		memcpy(&__dll_env, dll_env, sizeof(ACL_DLL_ENV));
	else
		memset(&__dll_env, 0, sizeof(ACL_DLL_ENV));
        if (__dll_env.logfp) {
                acl_msg_open2(__dll_env.logfp, "http-gb");
                acl_msg_info("%s(%d): logger opened, %s", __FUNCTION__,
                        __LINE__, ACL_VSTREAM_PATH(__dll_env.logfp));
        }

	/* 如果 mem_slice 非空则设置内存分配采用切片分配方式 */
	if (__dll_env.mem_slice)
		acl_mem_slice_set(__dll_env.mem_slice);
}

void http_plugin_init(ACL_DLL_ENV *dll_env, const char *cfg_dir)
{
	plugin_init(dll_env);
	http_conf_load(cfg_dir);
}

void http_plugin_pool_create(int threads_limit, int threads_idle)
{
	const char *myname = "http_plugin_pool_create";

	if (__thrpool != NULL) {
		acl_msg_error("%s(%d): thread pool has been created!",
			myname, __LINE__);
		return;
	}
	/* 创建线程池 */
	__thrpool = acl_thread_pool_create(threads_limit, threads_idle);
}

void http_plugin_pool_append(void (*start_routine)(void *), void *arg)
{
	acl_pthread_pool_add(__thrpool, start_routine, arg);
}

void http_plugin_debug_memory(int level)
{
	/* 是否调试插件的内存分配情况 */

	switch (level) {
	case 1:
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
		break;
	case 2:
		__dll_env.mmd = acl_debug_malloc_init(__dll_env.mmd,
					"access_log.txt");
		break;
	case 3:
		__dll_env.mmd = acl_debug_malloc_init(__dll_env.mmd,
					"access_log.txt");
		acl_memory_debug_start();
		acl_memory_debug_stack(1);
		break;
	default:
		break;
	}
}
