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
#include "stdlib/unix/acl_watchdog.h"
#include "net/acl_sane_socket.h"
#include "net/acl_vstream_net.h"
#include "event/acl_events.h"

/* Global library. */

#include "../master_flow.h"
#include "../master_params.h"
#include "../master_proto.h"

/* Application-specific */
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
	{ ACL_VAR_UDP_BUF_SIZE, ACL_DEF_UDP_BUF_SIZE, &acl_var_udp_buf_size, 0, 0 },
	{ ACL_VAR_UDP_RW_TIMEOUT, ACL_DEF_UDP_RW_TIMEOUT, &acl_var_udp_rw_timeout, 0, 0 },
	{ ACL_VAR_UDP_IDLE_LIMIT, ACL_DEF_UDP_IDLE_LIMIT, &acl_var_udp_idle_limit, 0, 0 },
	{ ACL_VAR_UDP_DELAY_SEC, ACL_DEF_UDP_DELAY_SEC, &acl_var_udp_delay_sec, 0, 0 },
	{ ACL_VAR_UDP_DELAY_USEC, ACL_DEF_UDP_DELAY_USEC, &acl_var_udp_delay_usec, 0, 0 },
	{ ACL_VAR_UDP_DAEMON_TIMEOUT, ACL_DEF_UDP_DAEMON_TIMEOUT, &acl_var_udp_daemon_timeout, 0, 0 },
	{ ACL_VAR_UDP_MASTER_MAXPROC, ACL_DEF_UDP_MASTER_MAXPROC, &acl_var_udp_master_maxproc, 0, 0},
	{ ACL_VAR_UDP_ENABLE_CORE, ACL_DEF_UDP_ENABLE_CORE, &acl_var_udp_enable_core, 0, 0 },
	{ ACL_VAR_UDP_MAX_DEBUG, ACL_DEF_UDP_MAX_DEBUG, &acl_var_udp_max_debug, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

char *acl_var_udp_queue_dir;
char *acl_var_udp_owner;
char *acl_var_udp_pid_dir;
char *acl_var_udp_event_mode;
char *acl_var_udp_log_debug;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ ACL_VAR_UDP_QUEUE_DIR, ACL_DEF_UDP_QUEUE_DIR, &acl_var_udp_queue_dir },
	{ ACL_VAR_UDP_OWNER, ACL_DEF_UDP_OWNER, &acl_var_udp_owner },
	{ ACL_VAR_UDP_PID_DIR, ACL_DEF_UDP_PID_DIR, &acl_var_udp_pid_dir },
	{ ACL_VAR_UDP_EVENT_MODE, ACL_DEF_UDP_EVENT_MODE, &acl_var_udp_event_mode },
	{ ACL_VAR_UDP_LOG_DEBUG, ACL_DEF_UDP_LOG_DEBUG, &acl_var_udp_log_debug },

        { 0, 0, 0 },
};

 /*
  * Global state.
  */
static int __socket_count = 1;

static ACL_EVENT *__event = NULL;
static ACL_VSTREAM **__servers = NULL;

static ACL_UDP_SERVER_FN __service_main;
static ACL_MASTER_SERVER_EXIT_FN __service_onexit;
static char *__service_name;
static char **__service_argv;
static void *__service_ctx;

static unsigned udp_server_generation;

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

static void close_listen_timer(int type acl_unused,
	ACL_EVENT *event, void *context acl_unused)
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

static void disable_listen(ACL_EVENT *event)
{
	if (__servers == NULL)
		return;

	/**
	 * 只所以采用定时器关闭监听流，一方面因为监听流在事件集合中是“常驻留”的，
	 * 另一方面本线程与事件循环主线程是不同的线程空间，如果在本线程直接关闭
	 * 监听流，会造成事件循环主线程在 select() 时报描述符非法，而当加了定时器
	 * 关闭方法后，定时器的运行线程空间与事件循环的运行线程空间是相同的，所以
	 * 不会造成冲突。这主要因为事件循环线程中先执行 select(), 后执行定时器，如果
	 * select() 执行后定时器启动并将监听流从事件集合中删除，则即使该监听流已经
	 * 准备好也会因其从事件集合中被删除而不会被触发，这样在下次事件循环时
	 * select() 所调用的事件集合中就不存在该监听流了。
	 */
	acl_event_request_timer(event, close_listen_timer, event, 1000000, 0);
}

/* udp_server_exit - normal termination */

static void udp_server_exit(void)
{
	if (__service_onexit)
		__service_onexit(__service_ctx);

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

static void udp_server_execute(ACL_EVENT *event, ACL_VSTREAM *stream)
{
	if (acl_var_udp_master_maxproc > 1
	    && acl_master_notify(acl_var_udp_pid, udp_server_generation,
		ACL_MASTER_STAT_TAKEN) < 0)
	{
		udp_server_abort(ACL_EVENT_NULL_TYPE, event, stream, event);
	}

	/* 回调用户注册的处理过程 */
	__service_main(stream, __service_name, __service_argv);

	/* 清除发生在 UDP 套接字上的临时性错误，以免事件引擎报错 */
	stream->flag = 0;

	if (acl_var_udp_master_maxproc > 1
	    && acl_master_notify(acl_var_udp_pid, udp_server_generation,
		ACL_MASTER_STAT_AVAIL) < 0)
	{
		udp_server_abort(ACL_EVENT_NULL_TYPE, event, stream, event);
	}
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
		acl_msg_fatal("%s, %s(%d): unknown event_type(%d, %d)",
			__FILE__, myname, __LINE__, event_type, ACL_EVENT_XCPT);

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

	acl_master_vars_init(acl_var_udp_buf_size, acl_var_udp_rw_timeout);
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

static void usage(int argc, char *argv[])
{
	int   i;
	char *service_name;

	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc(%d) invalid",
			__FILE__, __LINE__, argc);

	service_name = acl_mystrdup(acl_safe_basename(argv[0]));

	for (i = 0; i < argc; i++) {
		acl_msg_info("argv[%d]: %s", i, argv[i]);
	}

	acl_msg_info("usage: %s -h[help]"
		" -c [use chroot]"
		" -d [debug]"
		" -l [run alone]"
		" -n service_name"
		" -s socket_count"
		" -i [use stdin]"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -v [on acl_msg_verbose]"
		" -z [unlimit process count]"
		" -f conf_file",
		service_name);
}

/* acl_udp_server_main - the real main program */

void acl_udp_server_main(int argc, char **argv, ACL_UDP_SERVER_FN service, ...)
{
	const char *myname = "acl_udp_server_main";
	ACL_VSTREAM *stream = 0;
	char   *root_dir = 0;
	char   *user_name = 0;
	char   *service_name = acl_mystrdup(acl_safe_basename(argv[0]));
	int     c;
	va_list ap;
	ACL_MASTER_SERVER_INIT_FN pre_init = 0;
	ACL_MASTER_SERVER_INIT_FN post_init = 0;
	int     key;
	char   *transport = 0;
	int     alone = 0;
	int     zerolimit = 0;
	char   *generation;
	int     fd, i, fdtype = 0;
	int     event_mode;

	int     f_flag = 0;
	char    conf_file[1024];

	/*
	 * Pick up policy settings from master process. Shut up error messages to
	 * stderr, because no-one is going to see them.
	 */
	opterr = 0;
	while ((c = getopt(argc, argv, "hcdlm:n:o:s:it:uvzf:")) > 0) {
		switch (c) {
		case 'h':
			usage(argc, argv);
			exit (0);
		case 'f':
			acl_app_conf_load(optarg);
			f_flag = 1;
			ACL_SAFE_STRNCPY(conf_file, optarg, sizeof(conf_file));
			break;
		case 'c':
			root_dir = "setme";
			break;
		case 'l':
			alone = 1;
			break;
		case 'n':
			service_name = optarg;
			break;
		case 's':
			if ((__socket_count = atoi(optarg)) <= 0)
				acl_msg_fatal("invalid socket_count: %s", optarg);
			break;
		case 'i':
			stream = ACL_VSTREAM_IN;
			break;
		case 'u':
			user_name = "setme";
			break;
		case 't':
			transport = optarg;
			break;
		case 'v':
			acl_msg_verbose++;
			break;
		case 'z':
			zerolimit = 1;
			break;
		default:
			break;
		}
	}

	if (stream == NULL)
		udp_server_init(argv[0]);

	if (f_flag == 0)
		acl_msg_fatal("%s(%d)->%s: need \"-f pathname\"",
			__FILE__, __LINE__, myname);
	else if (acl_msg_verbose)
		acl_msg_info("%s(%d)->%s: configure file = %s", 
			__FILE__, __LINE__, myname, conf_file);

	/* Application-specific initialization. */

	va_start(ap, service);
	while ((key = va_arg(ap, int)) != 0) {
		switch (key) {
		case ACL_MASTER_SERVER_INT_TABLE:
			acl_get_app_conf_int_table(va_arg(ap, ACL_CONFIG_INT_TABLE *));
			break;
		case ACL_MASTER_SERVER_INT64_TABLE:
			acl_get_app_conf_int64_table(va_arg(ap, ACL_CONFIG_INT64_TABLE *));
			break;
		case ACL_MASTER_SERVER_STR_TABLE:
			acl_get_app_conf_str_table(va_arg(ap, ACL_CONFIG_STR_TABLE *));
			break;
		case ACL_MASTER_SERVER_BOOL_TABLE:
			acl_get_app_conf_bool_table(va_arg(ap, ACL_CONFIG_BOOL_TABLE *));
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
			__service_onexit = va_arg(ap, ACL_MASTER_SERVER_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_SOLITARY:
			if (!alone)
				acl_msg_fatal("service %s requires a process limit of 1",
					service_name);
			break;
		case ACL_MASTER_SERVER_UNLIMITED:
			if (!zerolimit)
				acl_msg_fatal("service %s requires a process limit of 0",
					service_name);
			break;
		default:
			acl_msg_panic("%s: unknown argument type: %d", myname, key);
		}
	}
	va_end(ap);

	if (root_dir)
		root_dir = acl_var_udp_queue_dir;
	if (user_name)
		user_name = acl_var_udp_owner;

	/* If not connected to stdin, stdin must not be a terminal. */
	if (stream == 0 && isatty(STDIN_FILENO))
		acl_msg_fatal("%s(%d)->%s: do not run this command by hand",
			__FILE__, __LINE__, myname);

	/* Can options be required? */
	if (stream == 0) {
		if (transport == 0)
			acl_msg_fatal("no transport type specified");
		if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_UDP) == 0)
			fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
		else
			acl_msg_fatal("unsupported transport type: %s", transport);
	}

	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &udp_server_generation);
		if (acl_msg_verbose)
			acl_msg_info("process generation: %s (%o)",
				generation, udp_server_generation);
	}

	/*
	 * Traditionally, BSD select() can't handle udpple processes selecting
	 * on the same socket, and wakes up every process in select(). See TCP/IP
	 * Illustrated volume 2 page 532. We avoid select() collisions with an
	 * external lock file.
	 */

	/* 设置回回调过程相关参数 */
	__service_main = service;
	__service_name = service_name;
	__service_argv = argv + optind;

	/*******************************************************************/

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

	/* 在切换用户运行身份前切换程序运行目录 */
	if (chdir(acl_var_udp_queue_dir) < 0)
		acl_msg_fatal("chdir(\"%s\"): %s",
			acl_var_udp_queue_dir, acl_last_serror());

	/* 切换用户运行身份前回调应用设置的回调函数 */
	if (pre_init)
		pre_init(__service_ctx);

	acl_chroot_uid(root_dir, user_name);

	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_udp_enable_core)
		acl_set_core_limit(0);
	udp_server_open_log();
	log_event_mode(event_mode);

	/*******************************************************************/

	/*
	 * Are we running as a one-shot server with the client connection on
	 * standard input? If so, make sure the output is written to stdout
	 * so as to satisfy common expectation.
	 */
	if (stream != 0) {
		/* Run post-jail initialization. */
		if (post_init)
			post_init(__service_ctx);

		service(stream, __service_name, __service_argv);
		udp_server_exit();
	}

	/*******************************************************************/

	/*
	 * Running as a semi-resident server. Service connection requests.
	 * Terminate when we have serviced a sufficient number of clients, when
	 * no-one has been talking to us for a configurable amount of time, or
	 * when the master process terminated abnormally.
	 */
	if (acl_var_udp_idle_limit > 0)
		acl_event_request_timer(__event, udp_server_timeout,
			__event, (acl_int64) acl_var_udp_idle_limit * 1000000, 0);

	/* socket count is as same listen_fd_count in parent process */

	__servers = (ACL_VSTREAM **)
		acl_mycalloc(__socket_count + 1, sizeof(ACL_VSTREAM *));
	for (i = 0; i < __socket_count + 1; i++)
		__servers[i] = NULL;

	i = 0;
	fd = ACL_MASTER_LISTEN_FD;
	for (; fd < ACL_MASTER_LISTEN_FD + __socket_count; fd++) {
		char  addr[64];

		stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_udp_buf_size,
			acl_var_udp_rw_timeout, fdtype);
		if (stream == NULL)
			acl_msg_fatal("%s(%d)->%s: stream null, fd = %d",
				__FILE__, __LINE__, myname, fd);

		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(stream, addr);
		acl_vstream_set_udp_io(stream);
		acl_non_blocking(fd, ACL_NON_BLOCKING);
		acl_event_enable_read(__event, stream, 0, udp_server_read, stream);
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		__servers[i++] = stream;
	}

	acl_event_enable_read(__event, ACL_MASTER_STAT_STREAM, 0,
		udp_server_abort, __event);
	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	/* 进程初始化完毕后回调此函数，以使用户可以初始化自己的环境 */
	if (post_init)
		post_init(__service_ctx);

	acl_msg_info("%s: daemon started, log: %s", argv[0], acl_var_udp_log_file);

	while (1)
		acl_event_loop(__event);

	/* not reached here */
	udp_server_exit();
}

#endif /* ACL_UNIX */
