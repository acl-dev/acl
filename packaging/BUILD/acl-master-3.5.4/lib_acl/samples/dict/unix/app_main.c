#include "lib_acl.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "app_log.h"
#include "app_main.h"

typedef struct APP_HANDLE {
	char  name[256];
} APP_HANDLE;

static APP_RUN_FN __run_fn = NULL;
static void *__run_ctx = NULL;

/* 客户端IO超时时间值 */
int app_var_client_idle_limit = 60;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ "app_client_idle_limit", 120, &app_var_client_idle_limit, 0, 0 },
	{ 0, 0, 0, 0, 0 },
};

static char *__default_deny_info = "You are not welcome!\r\n";
static char *__deny_info;
/*----------------------------------------------------------------------------*/

/**
 * 创建一个服务器框架
 * @return APP_HANDLE* 用户自己的应用数据句柄.
 */

static APP_HANDLE *app_create(void)
{
	char  myname[] = "app_create";
	APP_HANDLE *app;

	app = (APP_HANDLE *) acl_mycalloc(1, sizeof(APP_HANDLE));
	if (app == NULL)
		acl_msg_fatal("%s(%d): calloc error(%s)",
				myname, __LINE__, strerror(errno));

	ACL_SAFE_STRNCPY(app->name, myname, sizeof(app->name));

	return (app);
}
/*----------------------------------------------------------------------------*/
static void __read_notify_callback(int event_type,
				ACL_IOCTL *h_ioctl,
				ACL_VSTREAM *cstream,
				void *context)
{
	char  myname[] = "__read_notify_callback";
	APP_HANDLE *app;
	int   ret;

	app = (APP_HANDLE *) context;

	switch (event_type) {
	case ACL_EVENT_READ:
		ret = __run_fn(cstream, __run_ctx);
		if (ret < 0) {
			acl_vstream_close(cstream);
		} else if (ret == 0) {
			if (acl_msg_verbose)
				acl_msg_info("enable(%s), fd=%d, h_ioctl(%p)",
					myname, ACL_VSTREAM_SOCK(cstream), (void*) h_ioctl);
			acl_ioctl_enable_read(h_ioctl,
						cstream,
						app_var_client_idle_limit,
						__read_notify_callback,
						(void *) app);

		}
		break;
	case ACL_EVENT_RW_TIMEOUT:
	case ACL_EVENT_XCPT:
		acl_vstream_close(cstream);
		break;
	default:
		acl_msg_fatal("%s, %s(%d): unknown event type(%d)",
				__FILE__, myname, __LINE__, event_type);
		/* not reached */
		break;
	}
	if (acl_msg_verbose)
		acl_msg_info("%s(%d): total alloc: %d",
			myname, __LINE__, acl_mempool_total_allocated());
}

/**
 * 向任务池中添加一个工作任务
 * @param h_ioctl 服务器任务池句柄
 * @param app_handle 用户自己的应用数据句柄.
 * @param cstream 客户端数据流指针
 * 注: cstream 数据流会在该函数内部的回调函数中进行关闭, 所以该函数的调用者不要
 *     关闭该流.
 */

static void app_add_worker(ACL_IOCTL *h_ioctl, APP_HANDLE *app, ACL_VSTREAM *cstream)
{
	const char *myname = "app_add_worker";

	if (h_ioctl == NULL || cstream == NULL || app == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);
	
	/* 将客户端数据流的状态置入事件监控集合中 */
	if (acl_msg_verbose)
		acl_msg_info("%s(%d): ioctl=%p, fd=%d",
			myname, __LINE__, (void*) h_ioctl, ACL_VSTREAM_SOCK(cstream));

	acl_ioctl_enable_read(h_ioctl,
				cstream,
				app_var_client_idle_limit,
				__read_notify_callback,
				(void *) app);
}
/*---------------------- app main functions ----------------------------------*/

static int  __mempool_limit = 0;

static APP_HANDLE *__app_handle = NULL;

static APP_INIT_FN __app_init_fn = NULL;
static void *__app_init_ctx = NULL;
static APP_EXIT_FN __app_exit_fn = NULL;
static void *__app_exit_ctx = NULL;
static APP_THREAD_INIT_FN __app_thread_init_fn = NULL;
static APP_THREAD_EXIT_FN __app_thread_exit_fn = NULL;

static void __service(ACL_IOCTL *h_ioctl, ACL_VSTREAM *stream,
		char *service acl_unused, char **argv acl_unused)
{
	char  myname[] = "__service";
	char  ip[64];

	/*
	 * Sanity check. This service takes no command-line arguments.
	 */
	if (argv[0])
		acl_msg_fatal("%s, %s(%d): unexpected command-line argument: %s, service=%s",
			__FILE__, myname, __LINE__, argv[0], service ? service : "null");
	acl_watchdog_pat();

	if (isatty(ACL_VSTREAM_SOCK(stream))) {
		stream->rw_timeout = app_var_client_idle_limit;
		(void) __run_fn(stream, __run_ctx);
		return;
	}

	if (acl_getpeername(ACL_VSTREAM_SOCK(stream), ip, sizeof(ip)) < 0) {
		acl_msg_warn("%s, %s(%d): can't get socket's ip",
				__FILE__, myname, __LINE__);
		acl_vstream_close(stream);
	} else if (!acl_access_permit(ip)) {
		acl_msg_warn("%s, %s(%d): ip(%s) be denied",
				__FILE__, myname, __LINE__, ip);
		(void) acl_vstream_writen(stream, __deny_info, strlen(__deny_info));
		acl_vstream_close(stream);
	} else {
		stream->rw_timeout = app_var_client_idle_limit;
		app_add_worker(h_ioctl, __app_handle, stream);
	}
}

static void __pre_accept(char *name acl_unused, char **argv acl_unused)
{
}

static void __pre_jail_init(char *name acl_unused, char **argv acl_unused)
{
	acl_get_app_conf_int_table(__conf_int_tab);
	/* 是否采用 libcore 的日志记录 */
#ifdef	HAS_LIB_CORE
# ifdef	USE_LIBCORE_LOG
	app_set_libcore_log();
# endif
#endif
}

static void __post_jail_init(char *name acl_unused, char **argv acl_unused)
{
	char  myname[] = "__post_jail_init";

	if (acl_var_ioctl_access_allow != NULL)
		acl_access_add(acl_var_ioctl_access_allow, ",", ":");

	__app_handle = app_create();
	if (__app_handle == NULL)
		acl_msg_fatal("%s(%d): ioctl_create error(%s)",
				myname, __LINE__, strerror(errno));

	if (__app_init_fn != NULL)
		__app_init_fn(__app_init_ctx);

	if (__mempool_limit > 0) {
		acl_msg_info("use mempool, size limit is %d, use mutex",
			__mempool_limit);
	}

	/* TODO: you can add some init functions here */
}

static void __app_on_exit(char *service acl_unused, char **argv acl_unused)
{
	if (__app_exit_fn)
		__app_exit_fn(__app_exit_ctx);

#ifdef	HAS_LIB_CORE
# ifdef	USE_LIBCORE_LOG
	app_libcore_log_end();
# endif
#endif
}

static void __thread_init(void *arg)
{
	if (__app_thread_init_fn)
		__app_thread_init_fn(arg);
}

static void __thread_exit(void *arg)
{
	if (__app_thread_exit_fn)
		__app_thread_exit_fn(arg);
}

static void app_main_init(void)
{
	char *ptr, *pname;
	ACL_ARGV *env_argv;
	int   i;

	ptr = getenv("SERVICE_ENV");
	if (ptr == NULL || *ptr == 0)
		return;

	env_argv = acl_argv_split(ptr, ",\t ");
	if (env_argv == NULL)
		return;
	if (env_argv->argc == 0) {
		acl_argv_free(env_argv);
		return;
	}

	for (i = 0; i <env_argv->argc; i++) {
		pname = acl_argv_index(env_argv, i);
		ptr = strchr(pname, ':');
		if (ptr == NULL)
			continue;
		*ptr++ = 0;
		if (ptr == 0)
			continue;
		if (strcasecmp(pname, "mempool_limit") == 0) {
			__mempool_limit = atoi(ptr);
			break;
		}
	}

	acl_argv_free(env_argv);

	/* 因为是多线程程序，所以需要加互斥锁 */
	if (__mempool_limit > 0)
		acl_mempool_open(__mempool_limit, 1);

	__deny_info = __default_deny_info;
}

static ACL_CONFIG_BOOL_TABLE null_conf_bool_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0 },
};

static ACL_CONFIG_INT_TABLE null_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE null_conf_str_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0 },
};

void app_main(int argc, char *argv[], APP_RUN_FN run_fn, void *run_ctx, int name, ...)
{
	const char *myname = "app_main";
	va_list ap;
	ACL_CONFIG_BOOL_TABLE *bool_tab = null_conf_bool_tab;
	ACL_CONFIG_INT_TABLE *int_tab = null_conf_int_tab;
	ACL_CONFIG_STR_TABLE *str_tab = null_conf_str_tab;
	void *thread_init_ctx = NULL, *thread_exit_ctx = NULL;

	app_main_init();

	/* 提前进行模板初始化，以使日志尽早地打开 */
	acl_master_log_open(argv[0]);

	if (run_fn == NULL)
		acl_msg_fatal("%s: run_fn null", myname);

	__run_fn = run_fn;
	__run_ctx = run_ctx;

	va_start(ap, name);

	for (; name != APP_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case APP_CTL_INIT_FN:
			__app_init_fn = va_arg(ap, APP_INIT_FN);
			break;
		case APP_CTL_INIT_CTX:
			__app_init_ctx = va_arg(ap, void *);
			break;

		case APP_CTL_EXIT_FN:
			__app_exit_fn = va_arg(ap, APP_EXIT_FN);
			break;
		case APP_CTL_EXIT_CTX:
			__app_exit_ctx = va_arg(ap, void *);
			break;

		case APP_CTL_THREAD_INIT:
			__app_thread_init_fn = va_arg(ap, APP_THREAD_INIT_FN);
			break;
		case APP_CTL_THREAD_INIT_CTX:
			thread_init_ctx = va_arg(ap, void*);
			break;

		case APP_CTL_THREAD_EXIT:
			__app_thread_exit_fn = va_arg(ap, APP_THREAD_EXIT_FN);
			break;
		case APP_CTL_THREAD_EXIT_CTX:
			thread_exit_ctx = va_arg(ap, void*);
			break;

		case APP_CTL_CFG_BOOL:
			bool_tab = va_arg(ap, ACL_CONFIG_BOOL_TABLE *);
			break;
		case APP_CTL_CFG_INT:
			int_tab = va_arg(ap, ACL_CONFIG_INT_TABLE *);
			break;
		case APP_CTL_CFG_STR:
			str_tab = va_arg(ap, ACL_CONFIG_STR_TABLE *);
			break;
		case APP_CTL_DENY_INFO:
			__deny_info = acl_mystrdup(va_arg(ap, const char*));
			break;
		default:
			acl_msg_fatal("%s: bad name(%d)", myname, name);
		}
	}

	va_end(ap);

	acl_ioctl_server_main(argc, argv, __service,
				ACL_MASTER_SERVER_BOOL_TABLE, bool_tab,
				ACL_MASTER_SERVER_INT_TABLE, int_tab,
				ACL_MASTER_SERVER_STR_TABLE, str_tab,
				ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
				ACL_MASTER_SERVER_PRE_ACCEPT, __pre_accept,
				ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
				ACL_MASTER_SERVER_EXIT, __app_on_exit,
				ACL_MASTER_SERVER_THREAD_INIT, __thread_init,
				ACL_MASTER_SERVER_THREAD_EXIT, __thread_exit,
				ACL_MASTER_SERVER_THREAD_INIT_CTX, thread_init_ctx,
				ACL_MASTER_SERVER_THREAD_EXIT_CTX, thread_exit_ctx,
				0);
}
