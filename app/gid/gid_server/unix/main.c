#include "lib_acl.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <string.h>
#include <signal.h>
#include <assert.h>

#include "global.h"
#include "service_main.h"

static acl_pthread_pool_t *__thr_pool = NULL;
static int   __run_once = 0;

static void app_run(void *arg)
{
	ACL_VSTREAM *client = (ACL_VSTREAM*) arg;

	while (1) {
		if (service_main(NULL, client) < 0)
			break;
		if (__run_once)
			break;
	}

	printf("close client stream now\n");
	acl_vstream_close(client);
}

static void on_sigint(int signum acl_unused)
{
	signal(SIGINT, SIG_DFL);
	if (__thr_pool)
		acl_pthread_pool_destroy(__thr_pool);
	exit (0);
}

static ACL_VSTREAM *init(const char *filepath)
{
	ACL_VSTREAM *sstream;
	char  addr[] = ":7072";
	ACL_XINETD_CFG_PARSER *cfg = NULL;

	acl_lib_init();
	/*
	acl_debug_malloc_init(NULL, "log.txt");
	*/

	sstream = acl_vstream_listen(addr, 60);
	assert(sstream != NULL);
	printf("listen on: %s\n", addr);

	if (filepath)
		cfg = acl_xinetd_cfg_load(filepath);

	acl_xinetd_params_int_table(cfg, service_conf_int_tab);
	acl_xinetd_params_bool_table(cfg, service_conf_bool_tab);
	acl_xinetd_params_str_table(cfg, service_conf_str_tab);
	if (cfg)
		acl_xinetd_cfg_free(cfg);

	signal(SIGINT, on_sigint); /* 注册程序退出回调函数 */
	service_init(NULL);

	return (sstream);
}

static void end(void)
{
	service_exit(NULL);
	acl_myfree(var_cfg_gid_path);
	acl_lib_end();
}

static void app_test_thrpool(const char *filepath)
{
	ACL_VSTREAM *sstream;

	sstream = init(filepath);
	__thr_pool = acl_thread_pool_create(10, 120);

	while (1) {
		ACL_VSTREAM *client = acl_vstream_accept(sstream, NULL, 0);

		if (client == NULL) {
			printf("accept error: %s\n", acl_last_serror());
			break;
		} else {
			printf("accept one\n");
		}
		acl_pthread_pool_add(__thr_pool, app_run, client);
	}

	acl_vstream_close(sstream);
}

static void app_test_once(const char *filepath)
{
	ACL_VSTREAM *client, *sstream;

	__run_once = 1;
	sstream = init(filepath);
	client = acl_vstream_accept(sstream, NULL, 0);
	app_run(client);
	printf("close sstream now\n");
	acl_vstream_close(sstream);
	end();
}

static void usage(const char *procname)
{
	printf("usage: %s help\n", procname);
	printf("usage: %s test -f conf_file -m[use mempool]\n", procname);
}

static void app_test(const char *procname, int argc, char **argv)
{
	char  filepath[256];
	int   ch, use_mempool = 0, use_concurrent = 0;

	filepath[0] = 0;
	while ((ch = getopt(argc, argv, "f:mP")) > 0) {
		switch (ch) {
		case 'f':
			snprintf(filepath, sizeof(filepath), "%s", optarg);
			break;
		case 'm':
			use_mempool = 1;
			break;
		case 'P':
			use_concurrent = 1;
			break;
		default:
			break;
		}
	}

	if (filepath[0] == 0) {
		usage(procname);
		return;
	}
	if (use_mempool)
		acl_mem_slice_init(8, 1024, 100000,
			ACL_SLICE_FLAG_GC2 |
			ACL_SLICE_FLAG_RTGC_OFF |
			ACL_SLICE_FLAG_LP64_ALIGN);

	acl_msg_stdout_enable(1);

	if (use_concurrent)
		app_test_thrpool(filepath);
	else
		app_test_once(filepath);
}

int main(int argc, char **argv)
{
	if (argc == 2 && strcmp(argv[1], "help") == 0) {
		usage(argv[0]);
	} else if (argc >= 3 && strcmp(argv[1], "test") == 0) {
		app_test(argv[0], argc - 1, argv + 1);
	} else {
		acl_ioctl_app_main(argc, argv, service_main, NULL,
			ACL_APP_CTL_INIT_FN, service_init,
			ACL_APP_CTL_EXIT_FN, service_exit,
			ACL_APP_CTL_CFG_INT, service_conf_int_tab,
			ACL_APP_CTL_CFG_STR, service_conf_str_tab,
			ACL_APP_CTL_CFG_BOOL, service_conf_bool_tab,
			ACL_APP_CTL_END);
	}

	return (0);
}
