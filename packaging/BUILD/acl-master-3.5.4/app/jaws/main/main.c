#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "service_conf.h"
#include "service_main.h"
#include "http_module.h"

static ACL_AIO *var_aio;
static char *var_cfg_event_mode;
static char *var_cfg_master_log;
static char *var_cfg_listen_addr;

static ACL_CONFIG_STR_TABLE __service_conf_str_tab[] = {
	/* TODO: you can add configure variables of (char *) type here */
	/* example: { "mysql_dbaddr", "127.0.0.1:3306", &var_cfg_mysql_dbaddr }, */

	{ "aio_event_mode", "poll", &var_cfg_event_mode },
	{ "master_log", "jaws.log", &var_cfg_master_log },
	{ "master_service", "127.0.0.1:8808", &var_cfg_listen_addr },

	{ 0, 0, 0 },
};

static int listen_callback(ACL_ASTREAM *sstream, void *context acl_unused)
{
	ACL_SOCKET fd;

	fd = acl_inet_accept(ACL_VSTREAM_SOCK(acl_aio_vstream(sstream)));
	if (fd == ACL_SOCKET_INVALID) {
		printf("accept error(%s)\n", acl_last_serror());
		return (-1);
	}
	service_main(fd, var_aio);
	return (0);
}

static void gc_timer(int event_type acl_unused, void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;

	acl_mem_slice_delay_destroy();
}

static void run_loop(ACL_AIO *aio, const char *listen_addr)
{
	const char *myname = "run_loop";
	ACL_VSTREAM *sstream = acl_vstream_listen(listen_addr, 128);
	ACL_ASTREAM *astream;

	if (sstream == NULL)
		acl_msg_fatal("%s(%d): listen(%s) error(%s)",
			myname, __LINE__, listen_addr, acl_last_serror());
	acl_msg_info("%s(%d): listen %s ok, (fd=%d).",
		myname, __LINE__, listen_addr, ACL_VSTREAM_SOCK(sstream));

	astream = acl_aio_open(aio, sstream);
	acl_aio_ctl(astream,
		ACL_AIO_CTL_LISTEN_FN, listen_callback,
		ACL_AIO_CTL_END);
	acl_aio_listen(astream);
	/* 设定定时器定时清理垃圾回收器 */
	acl_aio_request_timer(aio, gc_timer, aio, 2, 1);

	while (1) {
		acl_aio_loop(aio);
	}
}

#ifdef	ACL_MS_WINDOWS

#include <direct.h>

static void onexit_fn(void *arg)
{
	char *procname = (char*) arg;

	acl_msg_info("%s: exit now", procname);
	acl_myfree(procname);
}

static void exec_path(char *exe_path, int size)  
{
	char	exeFullPath[MAX_PATH];  
	char	szDriver[MAX_PATH];  
	char	szDir[MAX_PATH];  
	char	szFile[MAX_PATH];  
	char	szExt[MAX_PATH];   

	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	_splitpath(exeFullPath, szDriver, szDir, szFile, szExt);   
	_snprintf(exe_path, size, "%s%s", szDriver, szDir);
}

#endif

static void change_path(void)
{
#ifdef ACL_MS_WINDOWS
	char  path[MAX_PATH];

	exec_path(path, sizeof(path));
	_chdir(path);
#endif
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

static ACL_AIO *init(const char *procname, int use_slice, int open_log acl_unused, const char *conf)
{
	ACL_AIO *aio;
	int   event_mode;
	ACL_XINETD_CFG_PARSER *cfg;
	ACL_FIFO *modules;

	change_path();

	if (use_slice)
		var_mem_slice = acl_mem_slice_init(8, 10240, 100000,
			ACL_SLICE_FLAG_GC2 | ACL_SLICE_FLAG_RTGC_OFF | ACL_SLICE_FLAG_LP64_ALIGN);
	else
		var_mem_slice = NULL;

	/* 初始化 acl 库 */
	acl_init();

	/* 加载配置文件 */
	cfg = acl_xinetd_cfg_load(conf);
	if (cfg == NULL)
		acl_msg_warn("load cfg(%s) error(%s)",
			conf, acl_last_serror());
	acl_xinetd_params_str_table(cfg, service_conf_str_tab);
	acl_xinetd_params_str_table(cfg, __service_conf_str_tab);
	acl_xinetd_params_bool_table(cfg, service_conf_bool_tab);
	acl_xinetd_params_int_table(cfg, service_conf_int_tab);
	acl_xinetd_cfg_free(cfg);

#ifdef 	ACL_UNIX
	if (open_log)
#endif
	{
		acl_msg_open(var_cfg_master_log, "jaws");
	}

#ifdef	ACL_UNIX
	if (strcasecmp(var_cfg_event_mode, "kernel") == 0)
		event_mode = ACL_EVENT_KERNEL;
	else if (strcasecmp(var_cfg_event_mode, "poll") == 0)
		event_mode = ACL_EVENT_POLL;
	else
#endif
		event_mode = ACL_EVENT_SELECT;
	var_aio = aio = acl_aio_create(event_mode);

	modules = get_modules();
	service_init(aio, modules);

#ifdef	ACL_MS_WINDOWS
	acl_proctl_child(procname, onexit_fn, acl_mystrdup(procname));
#else
	(void) procname;
#endif
	return (aio);
}

static void end(void)
{
	service_exit();
	acl_end();
}

static void usage(const char *procname)
{
	printf("usage: %s -h[help] -m[use mem slice] -l[open log] -c conf_file\n", procname);
}

int main(int argc, char *argv[])
{
	ACL_AIO *aio;
	char  conf[256] = "conf/jaws.cf";
	int   ch, use_slice = 0, open_log = 0;

	while ((ch = getopt(argc, argv, "hc:ml")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return (0);
		case 'c':
			ACL_SAFE_STRNCPY(conf, optarg, sizeof(conf));
			break;
		case 'm':
			use_slice = 1;
			break;
		case 'l':
			open_log = 1;
			break;
		default:
			break;
		}
	}
	
	aio = init(argv[0], use_slice, open_log, conf);
	run_loop(aio, var_cfg_listen_addr);
	end();
	return (0);
}

