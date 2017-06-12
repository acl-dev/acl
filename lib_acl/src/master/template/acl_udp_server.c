/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#ifdef STRCASECMP_IN_STRINGS_H
#include <strings.h>
#endif
#include <time.h>
#include <pthread.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_chroot_uid.h"
#include "stdlib/unix/acl_core_limit.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_myflock.h"
#include "net/acl_sane_socket.h"
#include "net/acl_vstream_net.h"
#include "event/acl_events.h"

/* Application-specific */

#include "master/acl_master_flow.h"
#include "master/acl_master_proto.h"
#include "master/acl_udp_params.h"
#include "master/acl_server_api.h"
#include "master_log.h"

int   acl_var_udp_pid;
char *acl_var_udp_procname;
char *acl_var_udp_log_file;

int   acl_var_udp_buf_size;
int   acl_var_udp_rw_timeout;
int   acl_var_udp_idle_limit;
int   acl_var_udp_delay_sec;
int   acl_var_udp_delay_usec;
int   acl_var_udp_daemon_timeout;
int   acl_var_udp_master_maxproc;
int   acl_var_udp_enable_core;
int   acl_var_udp_max_debug;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ ACL_VAR_UDP_BUF_SIZE, ACL_DEF_UDP_BUF_SIZE,
		&acl_var_udp_buf_size, 0, 0 },
	{ ACL_VAR_UDP_RW_TIMEOUT, ACL_DEF_UDP_RW_TIMEOUT,
		&acl_var_udp_rw_timeout, 0, 0 },
	{ ACL_VAR_UDP_IDLE_LIMIT, ACL_DEF_UDP_IDLE_LIMIT,
		&acl_var_udp_idle_limit, 0, 0 },
	{ ACL_VAR_UDP_DELAY_SEC, ACL_DEF_UDP_DELAY_SEC,
		&acl_var_udp_delay_sec, 0, 0 },
	{ ACL_VAR_UDP_DELAY_USEC, ACL_DEF_UDP_DELAY_USEC,
		&acl_var_udp_delay_usec, 0, 0 },
	{ ACL_VAR_UDP_DAEMON_TIMEOUT, ACL_DEF_UDP_DAEMON_TIMEOUT,
		&acl_var_udp_daemon_timeout, 0, 0 },
	{ ACL_VAR_UDP_MASTER_MAXPROC, ACL_DEF_UDP_MASTER_MAXPROC,
		&acl_var_udp_master_maxproc, 0, 0},
	{ ACL_VAR_UDP_ENABLE_CORE, ACL_DEF_UDP_ENABLE_CORE,
		&acl_var_udp_enable_core, 0, 0 },
	{ ACL_VAR_UDP_MAX_DEBUG, ACL_DEF_UDP_MAX_DEBUG,
		&acl_var_udp_max_debug, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

char *acl_var_udp_queue_dir;
char *acl_var_udp_owner;
char *acl_var_udp_pid_dir;
char *acl_var_udp_event_mode;
char *acl_var_udp_log_debug;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ ACL_VAR_UDP_QUEUE_DIR, ACL_DEF_UDP_QUEUE_DIR,
		&acl_var_udp_queue_dir },
	{ ACL_VAR_UDP_OWNER, ACL_DEF_UDP_OWNER,
		&acl_var_udp_owner },
	{ ACL_VAR_UDP_PID_DIR, ACL_DEF_UDP_PID_DIR,
		&acl_var_udp_pid_dir },
	{ ACL_VAR_UDP_EVENT_MODE, ACL_DEF_UDP_EVENT_MODE,
		&acl_var_udp_event_mode },
	{ ACL_VAR_UDP_LOG_DEBUG, ACL_DEF_UDP_LOG_DEBUG,
		&acl_var_udp_log_debug },

        { 0, 0, 0 },
};

 /*
  * Global state.
  */
static ACL_UDP_SERVER_FN         __service_main;
static ACL_MASTER_SERVER_EXIT_FN __service_exit;

static int                       __socket_count = 1;
static ACL_EVENT                *__event = NULL;
static ACL_VSTREAM             **__servers = NULL;

static char                     *__service_name;
static char                    **__service_argv;
static void                     *__service_ctx;

void acl_udp_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, acl_int64 delay, int keep)
{
	acl_event_request_timer(__event, timer_fn, arg, delay, keep);
}

ACL_VSTREAM **acl_udp_server_streams()
{
	if (__servers == NULL)
		acl_msg_warn("server streams NULL!");
	return __servers;
}

void acl_udp_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn, void *arg)
{
	acl_event_cancel_timer(__event, timer_fn, arg);
}

ACL_EVENT *acl_udp_server_event()
{
	return __event;
}

static void disable_listen(ACL_EVENT *event)
{
	int   i;

	if (__servers == NULL)
		return;

	for (i = 0; __servers[i] != NULL; i++) {
		acl_event_disable_readwrite(event, __servers[i]);
		acl_vstream_close(__servers[i]);
		__servers[i] = NULL;
		acl_msg_info("All servers closed now!");
	}

	acl_myfree(__servers);
	__servers = NULL;
}

/* udp_server_exit - normal termination */

static void udp_server_exit(void)
{
	if (__service_exit)
		__service_exit(__service_ctx);

	exit(0);
}

/* udp_server_timeout - idle time exceeded */

static void udp_server_timeout(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context acl_unused)
{
	const char *myname = "udp_server_timeout";

	if (acl_msg_verbose)
		acl_msg_info("%s: idle timeout -- exiting", myname);

	udp_server_exit();
}

/* udp_server_abort - terminate after abnormal master exit */

static void udp_server_abort(int event_type acl_unused,
	ACL_EVENT *event, ACL_VSTREAM *stream acl_unused,
	void *context acl_unused)
{
	const char *myname = "udp_server_abort";

	disable_listen(event);
	acl_msg_info("%s: master disconnect -- exiting", myname);
	udp_server_exit();
}

/* udp_server_execute - in case (char *) != (struct *) */

static void udp_server_execute(ACL_EVENT *event acl_unused, ACL_VSTREAM *stream)
{
	/* 回调用户注册的处理过程 */
	__service_main(stream, __service_name, __service_argv);

	/* 清除发生在 UDP 套接字上的临时性错误，以免事件引擎报错 */
	stream->flag = 0;
}

static void udp_server_read(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context acl_unused)
{
	const char *myname = "udp_server_read";
	int     time_left = -1;

	if (__servers == NULL) {
		acl_msg_info("%s, %s(%d): Server stoping ...",
			__FILE__, myname, __LINE__);
		return;
	}

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type: %d",
			__FILE__, myname, __LINE__, event_type);

	if (acl_var_udp_idle_limit > 0)
		time_left = (int) ((acl_event_cancel_timer(event,
			udp_server_timeout, event) + 999999) / 1000000);
	else
		time_left = acl_var_udp_idle_limit;

	udp_server_execute(__event, stream);

	if (time_left > 0)
		acl_event_request_timer(event, udp_server_timeout,
			event, (acl_int64) time_left * 1000000, 0);
}

static void udp_server_init(const char *procname)
{
	const char *myname = "udp_server_init";
	static int inited = 0;

	if (inited)
		return;

	inited = 1;

	if (procname == NULL || *procname == 0)
		acl_msg_fatal("%s(%d); procname null", myname, __LINE__);

	/*
	 * Don't die when a process goes away unexpectedly.
	 */
	signal(SIGPIPE, SIG_IGN);

	/*
	 * Don't die for frivolous reasons.
	 */
#ifdef SIGXFSZ
	signal(SIGXFSZ, SIG_IGN);
#endif

	/*
	 * May need this every now and then.
	 */
	acl_var_udp_pid = getpid();
	acl_var_udp_procname = acl_mystrdup(acl_safe_basename(procname));

	acl_var_udp_log_file = getenv("SERVICE_LOG");
	if (acl_var_udp_log_file == NULL) {
		acl_var_udp_log_file = acl_mystrdup("acl_master.log");
		acl_msg_warn("%s(%d)->%s: can't get SERVICE_LOG's env value,"
			"use %s log", __FILE__, __LINE__, myname,
			acl_var_udp_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_str_table(__conf_str_tab);
}

static void udp_server_open_log(void)
{
	/* first, close the master's log */
	master_log_close();

	/* second, open the service's log */
	acl_msg_open(acl_var_udp_log_file, acl_var_udp_procname);

	if (acl_var_udp_log_debug && *acl_var_udp_log_debug
		&& acl_var_udp_max_debug >= 100)
	{
		acl_debug_init2(acl_var_udp_log_debug, acl_var_udp_max_debug);
	}
}

static void log_event_mode(int event_mode)
{
	const char *myname = "log_event_mode";

	switch (event_mode) {
	case ACL_EVENT_SELECT:
		acl_msg_info("%s(%d): use select event", myname, __LINE__);
		break;
	case ACL_EVENT_POLL:
		acl_msg_info("%s(%d): use poll event", myname, __LINE__);
		break;
	case ACL_EVENT_KERNEL:
		acl_msg_info("%s(%d): use kernel_event", myname, __LINE__);
		break;
	default:
		acl_msg_info("%s(%d): use select event", myname, __LINE__);
		break;
	}
}

#ifdef SO_REUSEPORT

static int host_port(char *buf, char **host, char **port)
{
	const char *ptr = acl_host_port(buf, host, "", port, (char*) NULL);

	if (ptr != NULL) {
		acl_msg_error("%s(%d): invalid addr %s, %s",
			__FILE__, __LINE__, buf, ptr);
		return -1;
	}

	if (*port == NULL || atoi(*port) < 0) {
		acl_msg_error("%s(%d): invalid port: %s, addr: %s",
			__FILE__, __LINE__, *port ? *port : "null", buf);
		return -1;
	}

	if (*host && **host == 0)
		*host = 0;
	if (*host == NULL)
		*host = "0";

	return 0;
}

static struct addrinfo *host_addrinfo(const char *addr)
{
	int    err;
	struct addrinfo hints, *res0;
	char  *buf = acl_mystrdup(addr), *host = NULL, *port = NULL;

	if (host_port(buf, &host, &port) < 0) {
		acl_myfree(buf);
		return NULL;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
#ifdef  ACL_MACOSX
	hints.ai_flags    = AI_DEFAULT;
#elif   defined(ACL_ANDROID)
	hints.ai_flags    = AI_ADDRCONFIG;
#elif defined(ACL_WINDOWS)
	hints.ai_protocol = IPPROTO_UDP;
# if _MSC_VER >= 1500
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#else
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif
	if ((err = getaddrinfo(host, port, &hints, &res0))) {
		acl_msg_error("%s(%d): getaddrinfo error %s, peer=%s",
			__FILE__, __LINE__, gai_strerror(err), host);
		acl_myfree(buf);
		return NULL;
	}

	acl_myfree(buf);
	return res0;
}

static int bind_one(struct addrinfo *res0, struct addrinfo **res)
{
	struct addrinfo *it;
	int   on, fd;

	for (it = res0; it != NULL ; it = it->ai_next) {
		fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
		if (fd == ACL_SOCKET_INVALID) {
			acl_msg_error("%s(%d): create socket %s",
				__FILE__, __LINE__, acl_last_serror());
			return ACL_SOCKET_INVALID;
		}

		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
			(const void *) &on, sizeof(on)) < 0)
		{
			acl_msg_warn("%s(%d): setsockopt(SO_REUSEADDR): %s",
				__FILE__, __LINE__, acl_last_serror());
		}

#if defined(SO_REUSEPORT)
		on = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT,
			(const void *) &on, sizeof(on)) < 0)
		{
			acl_msg_warn("%s(%d): setsocket(SO_REUSEPORT): %s",
				__FILE__, __LINE__, acl_last_serror());
		}
#endif

#ifdef ACL_WINDOWS
		if (bind(fd, it->ai_addr, (int) it->ai_addrlen) == 0)
#else
		if (bind(fd, it->ai_addr, it->ai_addrlen) == 0)
#endif
		{
			*res = it;
			return fd;
		}

		acl_msg_error("%s(%d): bind error %s",
			__FILE__, __LINE__, acl_last_serror());
		acl_socket_close(fd);
	}

	return ACL_SOCKET_INVALID;
}

static int bind_addr(const char *addr)
{
	struct addrinfo *res0, *res;
	int    fd;

	res0 = host_addrinfo(addr);
	if (res0 == NULL) {
		acl_msg_fatal("%s(%d): host_addrinfo NULL, addr=%s",
			__FILE__, __LINE__, addr);
	}

	fd = bind_one(res0, &res);
	freeaddrinfo(res0);

	if (fd == ACL_SOCKET_INVALID) {
		acl_msg_fatal("%s(%d): bind %s error %s",
			__FILE__, __LINE__, addr, acl_last_serror());
	}

	return fd;
}

#endif /* SO_REUSEPORT */

static int open_server(const char *service_name)
{
	int fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
	ACL_VSTREAM *stat_stream;
	int event_mode, i, fd;
	char addr[64];
#ifdef SO_REUSEPORT
	ACL_ARGV *tokens;
	ACL_ITER  iter;
#endif

	/* 根据配置内容创建对应的事件句柄 */
	if (strcasecmp(acl_var_udp_event_mode, "poll") == 0) {
		__event = acl_event_new_poll(acl_var_udp_delay_sec,
				acl_var_udp_delay_usec);
		event_mode = ACL_EVENT_POLL;
	} else if (strcasecmp(acl_var_udp_event_mode, "kernel") == 0) {
		__event = acl_event_new_kernel(acl_var_udp_delay_sec,
				acl_var_udp_delay_usec);
		event_mode = ACL_EVENT_KERNEL;
	} else {
		__event = acl_event_new_select(acl_var_udp_delay_sec,
				acl_var_udp_delay_usec);
		event_mode = ACL_EVENT_SELECT;
	}

	/*
	 * Running as a semi-resident server. Service connection requests.
	 * Terminate when we have serviced a sufficient number of clients,
	 * when no-one has been talking to us for a configurable amount of
	 * time, or when the master process terminated abnormally.
	 */
	if (acl_var_udp_idle_limit > 0)
		acl_event_request_timer(__event, udp_server_timeout, __event,
			(acl_int64) acl_var_udp_idle_limit * 1000000, 0);

	/* socket count is as same listen_fd_count in parent process */

	__servers = (ACL_VSTREAM **)
		acl_mycalloc(__socket_count + 1, sizeof(ACL_VSTREAM *));
	for (i = 0; i < __socket_count + 1; i++)
		__servers[i] = NULL;

#ifdef SO_REUSEPORT
	tokens = acl_argv_split(service_name, "\"',; \t\r\n");
	acl_foreach(iter, tokens) {
		ACL_VSTREAM *stream;
		const char *ptr = (char *) iter.data;

		fd = bind_addr(ptr);
		stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_udp_buf_size,
				acl_var_udp_rw_timeout, fdtype);

		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(stream, addr);
		acl_vstream_set_udp_io(stream);
		acl_non_blocking(fd, ACL_NON_BLOCKING);
		acl_event_enable_read(__event, stream, 0, udp_server_read, 0);
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		__servers[i++] = stream;
	}
	acl_argv_free(tokens);
#else
	(void) service_name;
	fd = ACL_MASTER_LISTEN_FD;
	for (i = 0; fd < ACL_MASTER_LISTEN_FD + __socket_count; fd++) {
		ACL_VSTREAM *stream = acl_vstream_fdopen(fd, O_RDWR,
			acl_var_udp_buf_size, acl_var_udp_rw_timeout, fdtype);

		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(stream, addr);
		acl_vstream_set_udp_io(stream);
		acl_non_blocking(fd, ACL_NON_BLOCKING);
		acl_event_enable_read(__event, stream, 0, udp_server_read, 0);
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		__servers[i++] = stream;
	}
#endif

	stat_stream = acl_vstream_fdopen(ACL_MASTER_STATUS_FD,
		O_RDWR, 8192, 0, ACL_VSTREAM_TYPE_SOCK);
	acl_event_enable_read(__event, stat_stream, 0,
		udp_server_abort, __event);

	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	return event_mode;
}

static void usage(int argc, char *argv[])
{
	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc %d", __FILE__, __LINE__, argc);

	acl_msg_info("usage: %s -h[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -v [on acl_msg_verbose]"
		" -f conf_file", argv[0]);
}

void acl_udp_server_main(int argc, char **argv, ACL_UDP_SERVER_FN service, ...)
{
	const char *myname = "acl_udp_server_main";
	char   *service_name = acl_mystrdup(acl_safe_basename(argv[0]));
	ACL_MASTER_SERVER_INIT_FN pre_init = 0;
	ACL_MASTER_SERVER_INIT_FN post_init = 0;
	char   *root_dir = 0, *user_name = 0, *transport = 0;
	int     c, key, event_mode;
	const char *conf_file_ptr = 0;
	va_list ap;

	/*
	 * Pick up policy settings from master process. Shut up error messages
	 * to stderr, because no-one is going to see them.
	 */
	opterr = 0;

	while ((c = getopt(argc, argv, "hcn:s:t:uvf:")) > 0) {
		switch (c) {
		case 'h':
			usage(argc, argv);
			exit (0);
		case 'f':
			acl_app_conf_load(optarg);
			conf_file_ptr = optarg;
			break;
		case 'c':
			root_dir = "setme";
			break;
		case 'n':
			service_name = optarg;
			break;
		case 's':
			if ((__socket_count = atoi(optarg)) > 0)
				break;
			acl_msg_fatal("invalid socket_count: %s", optarg);
		case 'u':
			user_name = "setme";
			break;
		case 't':
			transport = optarg;
			break;
		case 'v':
			acl_msg_verbose++;
			break;
		default:
			break;
		}
	}

	udp_server_init(argv[0]);

	if (conf_file_ptr == 0)
		acl_msg_fatal("%s(%d), %s: need \"-f pathname\"",
			__FILE__, __LINE__, myname);
	else if (acl_msg_verbose)
		acl_msg_info("%s(%d), %s: configure file=%s", 
			__FILE__, __LINE__, myname, conf_file_ptr);

	/*******************************************************************/

	/* Application-specific initialization. */

	va_start(ap, service);

	while ((key = va_arg(ap, int)) != 0) {
		switch (key) {
		case ACL_MASTER_SERVER_INT_TABLE:
			acl_get_app_conf_int_table(
				va_arg(ap, ACL_CONFIG_INT_TABLE *));
			break;
		case ACL_MASTER_SERVER_INT64_TABLE:
			acl_get_app_conf_int64_table(
				va_arg(ap, ACL_CONFIG_INT64_TABLE *));
			break;
		case ACL_MASTER_SERVER_STR_TABLE:
			acl_get_app_conf_str_table(
				va_arg(ap, ACL_CONFIG_STR_TABLE *));
			break;
		case ACL_MASTER_SERVER_BOOL_TABLE:
			acl_get_app_conf_bool_table(
				va_arg(ap, ACL_CONFIG_BOOL_TABLE *));
			break;

		case ACL_MASTER_SERVER_CTX:
			__service_ctx = va_arg(ap, void *);
			break;
		case ACL_MASTER_SERVER_PRE_INIT:
			pre_init = va_arg(ap, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_POST_INIT:
			post_init = va_arg(ap, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_EXIT:
			__service_exit = va_arg(ap, ACL_MASTER_SERVER_EXIT_FN);
			break;
		default:
			acl_msg_panic("%s: unknown type: %d", myname, key);
		}
	}

	va_end(ap);

	if (root_dir)
		root_dir = acl_var_udp_queue_dir;
	if (user_name)
		user_name = acl_var_udp_owner;

	/* If not connected to stdin, stdin must not be a terminal. */
	if (isatty(STDIN_FILENO))
		acl_msg_fatal("%s(%d)->%s: do not run this command by hand",
			__FILE__, __LINE__, myname);

	if (transport == 0)
		acl_msg_fatal("no transport type specified");

	/* 设置回回调过程相关参数 */
	__service_main = service;
	__service_name = service_name;
	__service_argv = argv + optind;

	/*******************************************************************/

	event_mode = open_server(service_name);

	/* 在切换用户运行身份前切换程序运行目录 */
	if (chdir(acl_var_udp_queue_dir) < 0)
		acl_msg_fatal("chdir(\"%s\"): %s",
			acl_var_udp_queue_dir, acl_last_serror());

	/* 切换用户运行身份前回调应用设置的回调函数 */
	if (pre_init)
		pre_init(__service_ctx);

	if (user_name)
		acl_chroot_uid(root_dir, user_name);

	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_udp_enable_core)
		acl_set_core_limit(0);

	udp_server_open_log();
	log_event_mode(event_mode);

	/* 进程初始化完毕后回调此函数，以使用户可以初始化自己的环境 */
	if (post_init)
		post_init(__service_ctx);

	acl_msg_info("%s: daemon started", argv[0]);

	while (1)
		acl_event_loop(__event);
}

#endif /* ACL_UNIX */
