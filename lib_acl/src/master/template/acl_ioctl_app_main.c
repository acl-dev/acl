#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "net/acl_sane_socket.h"
#include "net/acl_access.h"
#include "master/acl_ioctl_params.h"
#include "master/acl_master_type.h"
#include "master/acl_master_conf.h"

#endif

#ifdef	ACL_UNIX

#include "master/acl_app_main.h"

typedef struct ACL_APP_HANDLE {
	char  name[256];
} ACL_APP_HANDLE;

static ACL_IOCTL_RUN_FN __run_fn = NULL;
static void *__run_ctx = NULL;

static char *var_cfg_deny_banner;
static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "master_deny_banner", "You are not welcome!", &var_cfg_deny_banner },
	{ 0, 0, 0 }
};

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ 0, 0, 0, 0, 0 }
};

static char *__deny_info = NULL;
static int (*__app_on_timeout)(ACL_VSTREAM *stream, void*) = NULL;
static void (*__app_on_close)(ACL_VSTREAM *stream, void*) = NULL;
/*----------------------------------------------------------------------------*/

/**
 * 创建一个服务器框架
 * @return ACL_APP_HANDLE* 用户自己的应用数据句柄.
 */

static ACL_APP_HANDLE *app_create(void)
{
	const char *myname = "app_create";
	ACL_APP_HANDLE *app;

	app = (ACL_APP_HANDLE *) acl_mycalloc(1, sizeof(ACL_APP_HANDLE));
	if (app == NULL)
		acl_msg_fatal("%s(%d): calloc error(%s)",
			myname, __LINE__, strerror(errno));

	ACL_SAFE_STRNCPY(app->name, myname, sizeof(app->name));

	return (app);
}
/*----------------------------------------------------------------------------*/
static void __read_notify_callback(int event_type, ACL_IOCTL *h_ioctl,
	ACL_VSTREAM *cstream, void *context)
{
	const char *myname = "__read_notify_callback";
	ACL_APP_HANDLE *app;
	int   ret;

	app = (ACL_APP_HANDLE *) context;

	switch (event_type) {
	case ACL_EVENT_READ:
		ret = __run_fn(cstream, __run_ctx);
		if (ret < 0) {
			if (__app_on_close != NULL)
				__app_on_close(cstream, __run_ctx);
			acl_vstream_close(cstream);
		} else if (ret == 0) {
			if (acl_msg_verbose)
				acl_msg_info("enable(%s), fd=%d, h_ioctl(%p)",
					myname, ACL_VSTREAM_SOCK(cstream),
					(void*) h_ioctl);
			acl_ioctl_enable_read(h_ioctl, cstream, acl_var_ioctl_rw_timeout,
				__read_notify_callback, (void *) app);

		}
		break;
	case ACL_EVENT_RW_TIMEOUT:
		if (__app_on_timeout == NULL) {
			if (__app_on_close != NULL)
				__app_on_close(cstream, __run_ctx);
			acl_vstream_close(cstream);
		} else if (__app_on_timeout(cstream, __run_ctx) < 0) {
			if (__app_on_close != NULL)
				__app_on_close(cstream, __run_ctx);
			acl_vstream_close(cstream);
		} else {
			acl_ioctl_enable_read(h_ioctl, cstream, acl_var_ioctl_rw_timeout,
				__read_notify_callback, (void *) app);
		}
		break;
	case ACL_EVENT_XCPT:
		if (__app_on_close != NULL)
			__app_on_close(cstream, __run_ctx);
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

static void app_add_worker(ACL_IOCTL *h_ioctl, ACL_APP_HANDLE *app, ACL_VSTREAM *cstream)
{
	const char *myname = "app_add_worker";

	if (h_ioctl == NULL || cstream == NULL || app == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);
	
	/* 将客户端数据流的状态置入事件监控集合中 */
	if (acl_msg_verbose)
		acl_msg_info("%s(%d): ioctl=%p, fd=%d, timeout: %d",
			myname, __LINE__, (void*) h_ioctl, ACL_VSTREAM_SOCK(cstream),
			acl_var_ioctl_rw_timeout);

	acl_ioctl_enable_read(h_ioctl, cstream, acl_var_ioctl_rw_timeout,
		__read_notify_callback, (void *) app);
}
/*---------------------- app main functions ----------------------------------*/

static int  __mempool_limit = 0;

static ACL_APP_HANDLE *__app_handle = NULL;

static ACL_APP_INIT_FN __app_init_fn = NULL;
static void *__app_init_ctx = NULL;
static ACL_APP_EXIT_FN __app_exit_fn = NULL;
static void *__app_exit_ctx = NULL;
static ACL_APP_THREAD_INIT_FN __app_thread_init_fn = NULL;
static ACL_APP_THREAD_EXIT_FN __app_thread_exit_fn = NULL;
static ACL_APP_OPEN_LOG __app_open_log = NULL;
static ACL_APP_PRE_JAIL __app_pre_jail = NULL;
static void *__app_pre_jail_ctx = NULL;
static ACL_APP_CLOSE_LOG __app_close_log = NULL;
static int (*__app_on_accept)(ACL_VSTREAM *stream) = NULL;

static void __service(ACL_IOCTL *h_ioctl, ACL_VSTREAM *stream,
	char *service acl_unused, char **argv acl_unused)
{
	const char *myname = "__service";
	char  addr[64], *ptr;

	/*
	 * Sanity check. This service takes no command-line arguments.
	 */
	if (argv[0])
		acl_msg_fatal("%s, %s(%d): unexpected command-line argument: %s, service=%s",
			__FILE__, myname, __LINE__, argv[0], service ? service : "null");
	acl_watchdog_pat();

	if (isatty(ACL_VSTREAM_SOCK(stream))) {
		(void) __run_fn(stream, __run_ctx);
		return;
	}

	if (acl_getpeername(ACL_VSTREAM_SOCK(stream), addr, sizeof(addr)) < 0) {
		acl_msg_warn("%s, %s(%d): can't get socket's addr(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		acl_vstream_close(stream);
		return;
	}

	ptr = strchr(addr, ':');
	if (ptr)
		*ptr = 0;

	if (!acl_access_permit(addr)) {
		acl_msg_warn("%s, %s(%d): addr(%s) be denied",
			__FILE__, myname, __LINE__, addr);
		if (__deny_info && *__deny_info)
			(void) acl_vstream_fprintf(stream, "%s\r\n", __deny_info);
		acl_vstream_close(stream);
	} else {
		int ret = 0;
		if (__app_on_accept) {
			if ((ret = __app_on_accept(stream)) < 0)
				acl_vstream_close(stream);
		}
		if (ret == 0)
			app_add_worker(h_ioctl, __app_handle, stream);
	}
}

static void __pre_accept(char *name acl_unused, char **argv acl_unused)
{
}

static void __pre_jail_init(char *name acl_unused, char **argv acl_unused)
{
	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_str_table(__conf_str_tab);

	/* 当没有通过函数参数设置拒绝访问信息时，则使用配置文件中的内容 */
	if (__deny_info == NULL)
		__deny_info = var_cfg_deny_banner;

	/* 是否采用用户自定义的日志函数库 */
	if (__app_open_log)
		__app_open_log();

	if (__app_pre_jail)
		__app_pre_jail(__app_pre_jail_ctx);
}

static void __post_jail_init(char *name acl_unused, char **argv acl_unused)
{
	const char *myname = "__post_jail_init";

	if (acl_var_ioctl_access_allow != NULL)
		acl_access_add(acl_var_ioctl_access_allow, ", \t", ":");

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

	if (__app_close_log)
		__app_close_log();
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

	for (i = 0; i < env_argv->argc; i++) {
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
}

static ACL_CONFIG_BOOL_TABLE null_conf_bool_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0 },
};

static ACL_CONFIG_INT_TABLE null_conf_int_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_INT64_TABLE null_conf_int64_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0, 0, 0 },
};

static ACL_CONFIG_STR_TABLE null_conf_str_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0 },
};

void acl_ioctl_app_main(int argc, char *argv[],
	ACL_IOCTL_RUN_FN run_fn, void *run_ctx, int name, ...)
{
	const char *myname = "acl_ioctl_app_main";
	va_list ap;
	ACL_CONFIG_BOOL_TABLE *bool_tab = null_conf_bool_tab;
	ACL_CONFIG_INT_TABLE *int_tab = null_conf_int_tab;
	ACL_CONFIG_INT64_TABLE *int64_tab = null_conf_int64_tab;
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

	for (; name != ACL_APP_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_APP_CTL_ON_ACCEPT:
			__app_on_accept = va_arg(ap, int (*)(ACL_VSTREAM*));
			break;
		case ACL_APP_CTL_ON_TIMEOUT:
			__app_on_timeout = va_arg(ap, int (*)(ACL_VSTREAM*, void*));
			break;
		case ACL_APP_CTL_ON_CLOSE:
			__app_on_close = va_arg(ap, void (*)(ACL_VSTREAM*, void*));
			break;

		case ACL_APP_CTL_INIT_FN:
			__app_init_fn = va_arg(ap, ACL_APP_INIT_FN);
			break;
		case ACL_APP_CTL_INIT_CTX:
			__app_init_ctx = va_arg(ap, void *);
			break;

		case ACL_APP_CTL_EXIT_FN:
			__app_exit_fn = va_arg(ap, ACL_APP_EXIT_FN);
			break;
		case ACL_APP_CTL_EXIT_CTX:
			__app_exit_ctx = va_arg(ap, void *);
			break;

		case ACL_APP_CTL_THREAD_INIT:
			__app_thread_init_fn = va_arg(ap, ACL_APP_THREAD_INIT_FN);
			break;
		case ACL_APP_CTL_THREAD_INIT_CTX:
			thread_init_ctx = va_arg(ap, void*);
			break;

		case ACL_APP_CTL_THREAD_EXIT:
			__app_thread_exit_fn = va_arg(ap, ACL_APP_THREAD_EXIT_FN);
			break;
		case ACL_APP_CTL_THREAD_EXIT_CTX:
			thread_exit_ctx = va_arg(ap, void*);
			break;

		case ACL_APP_CTL_OPEN_LOG:
			__app_open_log = va_arg(ap, ACL_APP_OPEN_LOG);
			break;
		case ACL_APP_CTL_CLOSE_LOG:
			__app_close_log = va_arg(ap, ACL_APP_CLOSE_LOG);
			break;

		case ACL_APP_CTL_PRE_JAIL_CTX:
			__app_pre_jail_ctx = va_arg(ap, void*);
			break;
		case ACL_APP_CTL_PRE_JAIL:
			__app_pre_jail = va_arg(ap, ACL_APP_PRE_JAIL);
			break;

		case ACL_APP_CTL_CFG_BOOL:
			bool_tab = va_arg(ap, ACL_CONFIG_BOOL_TABLE *);
			break;
		case ACL_APP_CTL_CFG_INT:
			int_tab = va_arg(ap, ACL_CONFIG_INT_TABLE *);
			break;
		case ACL_APP_CTL_CFG_INT64:
			int64_tab = va_arg(ap, ACL_CONFIG_INT64_TABLE *);
			break;
		case ACL_APP_CTL_CFG_STR:
			str_tab = va_arg(ap, ACL_CONFIG_STR_TABLE *);
			break;
		case ACL_APP_CTL_DENY_INFO:
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
		ACL_MASTER_SERVER_INT64_TABLE, int64_tab,
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
#endif

