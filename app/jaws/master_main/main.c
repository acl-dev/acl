#include "lib_acl.h"
#include "app_log.h"
#include "http_service.h"
#include "service_conf.h"
#include "service_main.h"

static int __use_slice = 0;
static int __slice_base = 8;
static int __slice_gc = ACL_SLICE_FLAG_GC2;
static int __nslice = 1024;
static int __nalloc_gc = 100000;
static int __rtgc_off = 1;

static void parse_env(void)
{
	char *ptr, *name;
	ACL_ARGV *env_argv;
	ACL_ITER iter;

	ptr = getenv("SERVICE_ENV");
	if (ptr == NULL)
		return;

	env_argv = acl_argv_split(ptr, ",\t ");
	if (env_argv == NULL)
		return;

	acl_foreach(iter, env_argv) {
		name = (char*) iter.data;
		ptr = strchr(name, ':');
		if (ptr == NULL)
			continue;
		*ptr++ = 0;
		if (*ptr == 0)
			continue;
#define	STREQ	!strcasecmp

		if (STREQ(name, "use_slice")) {
			if (STREQ(ptr, "true"))
				__use_slice = 1;
			else
				__use_slice = 0;
		} else if (STREQ(name, "slice_base")) {
			__slice_base = atoi(ptr);
			if (__slice_base <= 0)
				__slice_base = 8;
		} else if (STREQ(name, "nslice")) {
			__nslice = atoi(ptr);
			if (__nslice <= 0)
				__nslice = 1024;
		} else if (STREQ(name, "nalloc_gc")) {
			__nalloc_gc = atoi(ptr);
			if (__nalloc_gc <= 0)
				__nalloc_gc = 100000;
		} else if (STREQ(name, "slice_gc")) {
			if (STREQ(ptr, "gc1"))
				__slice_gc = ACL_SLICE_FLAG_GC1;
			else if (STREQ(ptr, "gc3"))
				__slice_gc = ACL_SLICE_FLAG_GC3;
			else
				__slice_gc = ACL_SLICE_FLAG_GC2;
		} else if (STREQ(name, "rtgc_off")) {
			if (STREQ(ptr, "true"))
				__rtgc_off = 1;
			else
				__rtgc_off = 0;
		}
	}

	if (__slice_gc == 0)
		__use_slice = 0;
}

static ACL_FIFO *get_modules(void)
{
	ACL_FIFO *modules = NULL;

#ifdef	JAWS_STATIC
	MODULE_SERVICE *module;

	modules = acl_fifo_new();

	module = (MODULE_SERVICE*) acl_mycalloc(1, sizeof(MODULE_SERVICE));
	module->mod_init = module_service_init;
	module->mod_create = module_service_create;
	module->mod_main = module_service_main;
	acl_fifo_push(modules, module);
#endif
	return (modules);
}

static void __service_init(void *init_ctx acl_unused)
{
	ACL_AIO *aio = acl_aio_server_handle();
	ACL_FIFO *modules;

	modules = get_modules();
	service_init(aio, NULL);
}

static void __service_exit(void *exit_ctx acl_unused)
{
	service_exit();
}

static int __service_main(ACL_SOCKET fd, void *run_ctx acl_unused)
{
	ACL_AIO *aio = acl_aio_server_handle();

	return (service_main(fd, aio));
}

int main(int argc, char *argv[])
{
	parse_env();

	if (__use_slice) {
		acl_msg_info("use mem slice, slice_base(%d), nslice(%d),"
			" nalloc_gc(%d), rtgc_off(%s)", __slice_base,
			__nslice, __nalloc_gc, __rtgc_off ? "yes" : "no");
		var_mem_slice = acl_mem_slice_init(__slice_base, __nslice,
			__nalloc_gc, __rtgc_off ?
			__slice_gc | ACL_SLICE_FLAG_RTGC_OFF : __slice_gc);
	} else {
		var_mem_slice = NULL;
	}

	/* acl_debug_malloc_init("log.txt"); */
	acl_aio_app2_main(argc, argv, __service_main, NULL,
		ACL_APP_CTL_INIT_FN, __service_init,
		ACL_APP_CTL_EXIT_FN, __service_exit,
		ACL_APP_CTL_CFG_BOOL, service_conf_bool_tab,
		ACL_APP_CTL_CFG_INT, service_conf_int_tab,
		ACL_APP_CTL_CFG_STR, service_conf_str_tab,
/*
		ACL_APP_CTL_OPEN_LOG, app_set_libcore_log,
		ACL_APP_CTL_CLOSE_LOG, app_libcore_log_end,
*/
		ACL_APP_CTL_END);

	exit (0);
}

