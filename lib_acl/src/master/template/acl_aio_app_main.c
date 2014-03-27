#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "net/acl_sane_socket.h"
#include "net/acl_access.h"
#include "master/acl_aio_params.h"
#include "master/acl_master_type.h"
#include "master/acl_master_conf.h"

#endif

#ifdef	ACL_UNIX

#include "master/acl_app_main.h"

static int  __mempool_limit = 0;
static int  __mempool_use_mutex = 0;

static ACL_AIO_RUN_FN __run_fn = NULL;
static ACL_AIO_RUN2_FN __run2_fn = NULL;
static void *__run_ctx = NULL;
static ACL_APP_INIT_FN __app_init_fn = NULL;
static void *__app_init_ctx = NULL;
static ACL_APP_EXIT_FN __app_exit_fn = NULL;
static void *__app_exit_ctx = NULL;
static ACL_APP_OPEN_LOG __app_open_log = NULL;
static ACL_APP_CLOSE_LOG __app_close_log = NULL;
static ACL_APP_PRE_JAIL __app_pre_jail = NULL;
static void *__app_pre_jail_ctx = NULL;
static char *__deny_info = NULL;

static char *var_cfg_deny_banner;
static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	/* TODO: you can add configure variables of int type here */

	{ "master_deny_banner", "You are not welcome!", &var_cfg_deny_banner },
	{ 0, 0, 0 }
};

static void __vstream_on_close(ACL_VSTREAM *vstream acl_unused, void *context)
{
	ACL_ASTREAM *astream = (ACL_ASTREAM*) context;

	acl_aio_server_on_close(astream);
}

static void __service(ACL_SOCKET fd, char *service acl_unused,
	char **argv acl_unused)
{
	const char *myname = "__service";
	char  addr[64], *ptr, ip[64];

	/* Sanity check. This service takes no command-line arguments. */
	if (argv[0])
		acl_msg_fatal("%s, %s(%d): unexpected command-line "
			"argument: %s, service=%s", __FILE__, myname,
			__LINE__, argv[0], service ? service : "null");

	acl_watchdog_pat();

	if (isatty(fd)) {
		acl_msg_error("%s, %s(%d): fd isatty",
			__FILE__, myname, __LINE__);
		return;
	}

	if (acl_getpeername(fd, addr, sizeof(addr)) < 0) {
		acl_msg_warn("%s, %s(%d): can't get socket's addr",
			__FILE__, myname, __LINE__);
		acl_socket_close(fd);
		return;
	}

	ACL_SAFE_STRNCPY(ip, addr, sizeof(ip));
	ptr = strchr(ip, ':');
	if (ptr)
		*ptr = 0;

	if (!acl_access_permit(ip)) {
		acl_msg_warn("%s, %s(%d): addr(%s) be denied",
			__FILE__, myname, __LINE__, ip);

		if (__deny_info && *__deny_info) {
			if (write(fd, __deny_info, strlen(__deny_info)) > 0
				&& write(fd, "\r\n", 2) > 0)
			{
				/* do nothing, just avoid compile warning */
			}
		}

		acl_socket_close(fd);
	} else if (__run_fn != NULL) {
		ACL_VSTREAM *vstream;
		ACL_ASTREAM *astream;

		vstream = acl_vstream_fdopen(fd, O_RDWR, acl_var_aio_buf_size,
			0, ACL_VSTREAM_TYPE_SOCK);

		acl_vstream_set_peer(vstream, addr);
		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(vstream, addr);
		astream = acl_aio_open(acl_aio_server_handle(), vstream);

		acl_vstream_add_close_handle(vstream, __vstream_on_close, astream);

		if (__run_fn(astream, __run_ctx) != 0)
			acl_aio_iocp_close(astream);
	} else if (__run2_fn != NULL) {
		if (__run2_fn(fd, __run_ctx) != 0)
			acl_socket_close(fd);
	} else
		acl_msg_fatal("%s(%d), %s: __run_fn and __run2_fn are null",
			__FILE__, __LINE__, myname);
}

static void __pre_accept(char *name acl_unused, char **argv acl_unused)
{
}

static void __pre_jail_init(char *name acl_unused, char **argv acl_unused)
{
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

	if (acl_var_aio_access_allow != NULL)
		acl_access_add(acl_var_aio_access_allow, ", \t", ":");

	if (__app_init_fn != NULL)
		__app_init_fn(__app_init_ctx);

	if (__mempool_limit > 0) {
		acl_msg_info("use mempool, size limit is %d, %s mutex",
			__mempool_limit, __mempool_use_mutex ? "use" : "no");
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
		if (strcasecmp(pname, "mempool_limit") == 0)
			__mempool_limit = atoi(ptr);
		else if (strcasecmp(pname, "mempool_use_mutex") == 0) {
			if (strcasecmp(ptr, "true") == 0)
				__mempool_use_mutex = 1;
		}
	}

	acl_argv_free(env_argv);

	if (__mempool_limit > 0)
		acl_mempool_open(__mempool_limit, __mempool_use_mutex);
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
	/* TODO: you can add configure variables of acl_int64 type here */
	{ 0, 0, 0, 0, 0 },
};      

static ACL_CONFIG_STR_TABLE null_conf_str_tab[] = {
	/* TODO: you can add configure variables of int type here */
	{ 0, 0, 0 },
};

void acl_aio_app_main(int argc, char *argv[], ACL_AIO_RUN_FN run_fn,
	void *run_ctx, int name, ...)
{
	const char *myname = "acl_aio_app_main";
	va_list ap;
	ACL_CONFIG_BOOL_TABLE *bool_tab = null_conf_bool_tab;
	ACL_CONFIG_INT_TABLE *int_tab = null_conf_int_tab;
	ACL_CONFIG_INT64_TABLE *int64_tab = null_conf_int64_tab;
	ACL_CONFIG_STR_TABLE *str_tab = null_conf_str_tab;

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

		case ACL_APP_CTL_OPEN_LOG:
			__app_open_log = va_arg(ap, ACL_APP_OPEN_LOG);
			break;
		case ACL_APP_CTL_CLOSE_LOG:
			__app_close_log = va_arg(ap, ACL_APP_CLOSE_LOG);
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
			acl_msg_info("%s: bad name(%d)", myname, name);
		}
	}

	va_end(ap);

	acl_aio_server_main(argc, argv, __service,
			ACL_MASTER_SERVER_BOOL_TABLE, bool_tab,
			ACL_MASTER_SERVER_INT_TABLE, int_tab,
			ACL_MASTER_SERVER_INT64_TABLE, int64_tab,
			ACL_MASTER_SERVER_STR_TABLE, str_tab,
			ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
			ACL_MASTER_SERVER_PRE_ACCEPT, __pre_accept,
			ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
			ACL_MASTER_SERVER_EXIT, __app_on_exit,
			0);
}

void acl_aio_app2_main(int argc, char *argv[], ACL_AIO_RUN2_FN run2_fn,
	void *run_ctx, int name, ...)
{
	const char *myname = "acl_aio_app2_main";
	va_list ap;
	ACL_CONFIG_BOOL_TABLE *bool_tab = null_conf_bool_tab;
	ACL_CONFIG_INT_TABLE *int_tab = null_conf_int_tab;
	ACL_CONFIG_INT64_TABLE *int64_tab = null_conf_int64_tab;
	ACL_CONFIG_STR_TABLE *str_tab = null_conf_str_tab;

	app_main_init();
	/* 提前进行模板初始化，以使日志尽早地打开 */
	acl_master_log_open(argv[0]);

	if (run2_fn == NULL)
		acl_msg_fatal("%s: run_fn null", myname);

	__run2_fn = run2_fn;
	__run_ctx = run_ctx;

	va_start(ap, name);

	for (; name != ACL_APP_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
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
			acl_msg_info("%s: bad name(%d)", myname, name);
		}
	}

	va_end(ap);

	acl_aio_server_main(argc, argv, __service,
			ACL_MASTER_SERVER_BOOL_TABLE, bool_tab,
			ACL_MASTER_SERVER_INT_TABLE, int_tab,
			ACL_MASTER_SERVER_INT64_TABLE, int64_tab,
			ACL_MASTER_SERVER_STR_TABLE, str_tab,
			ACL_MASTER_SERVER_PRE_INIT, __pre_jail_init,
			ACL_MASTER_SERVER_PRE_ACCEPT, __pre_accept,
			ACL_MASTER_SERVER_POST_INIT, __post_jail_init,
			ACL_MASTER_SERVER_EXIT, __app_on_exit,
			0);
}
#endif
