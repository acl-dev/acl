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

#endif /* ACL_UNIX */

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_chroot_uid.h"
#include "stdlib/unix/acl_core_limit.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/acl_argv.h"
#include "net/acl_sane_socket.h"
#include "net/acl_vstream_net.h"
#include "net/acl_ifconf.h"
#include "net/acl_listen.h"
#include "event/acl_events.h"

/* Application-specific */

#include "master/acl_master_flow.h"
#include "master/acl_master_proto.h"
#include "master/acl_udp_params.h"
#include "master/acl_server_api.h"
#include "master/acl_master_type.h"
#include "master/acl_master_conf.h"
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
int   acl_var_udp_threads;

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
	{ ACL_VAR_UDP_THREADS, ACL_DEF_UDP_THREADS,
		&acl_var_udp_threads, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

long long int acl_var_udp_use_limit;

static ACL_CONFIG_INT64_TABLE __conf_int64_tab[] = {
	{ ACL_VAR_UDP_USE_LIMIT, ACL_DEF_UDP_USE_LIMIT,
		&acl_var_udp_use_limit, 0, 0 },

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

typedef struct SERVER {
	acl_pthread_t tid;
	ACL_EVENT    *event;
	ACL_VSTREAM **streams;
	int           socket_count;
} SERVER;

 /*
  * Global state.
  */
static ACL_UDP_SERVER_FN                __service_main;
static ACL_MASTER_SERVER_EXIT_FN        __service_exit;
static ACL_MASTER_SERVER_THREAD_INIT_FN __thread_init;
static ACL_MASTER_SERVER_THREAD_EXIT_FN __thread_exit;
static void             *__thread_init_ctx = NULL;
static void             *__thread_exit_ctx = NULL;

static int               __event_mode;
static int               __socket_count = 1;
static SERVER           *__servers = NULL;
static __thread SERVER  *__server = NULL;
static ACL_EVENT        *__main_event = NULL;

static const char        *__service_name;
static char             **__service_argv;
static void              *__service_ctx;
static int                __daemon_mode = 1;

typedef struct {
	long long   last;
	long long   used;
	ACL_ATOMIC *time_atomic;
	ACL_ATOMIC *used_atomic;
} RUNNING_STATUS;

static RUNNING_STATUS     *__status = NULL;

static void running_status_init(void)
{
	acl_assert(__status == NULL);
	__status = (RUNNING_STATUS *) acl_mymalloc(sizeof(RUNNING_STATUS));

	__status->last        = (long long) time(NULL);
	__status->used        = 0;
	__status->time_atomic = acl_atomic_new();
	__status->used_atomic = acl_atomic_new();
	acl_atomic_set(__status->time_atomic, &__status->last);
	acl_atomic_set(__status->used_atomic, &__status->used);
}

static void running_status_free(void)
{
	acl_assert(__status);
	acl_atomic_free(__status->time_atomic);
	acl_atomic_free(__status->used_atomic);
	acl_myfree(__status);
	__status = NULL;
}

static long long status_last(void)
{
	acl_assert(__status);
	return __status->last;
}

static long long status_increase_used(void)
{
	long long n;

	acl_assert(__status);
	n = acl_atomic_int64_add_fetch(__status->used_atomic, 1);
	if (n < 0)
		acl_atomic_int64_set(__status->used_atomic, 0);

	acl_atomic_int64_set(__status->time_atomic, (long long) time(NULL));
	return n;
}

ACL_EVENT *acl_udp_server_event(void)
{
	if (acl_pthread_self() == acl_main_thread_self()) {
		acl_assert(__main_event);
		return __main_event;
	} else {
		acl_assert(__server);
		return __server->event;
	}
}

void acl_udp_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, acl_int64 delay, int keep)
{
	acl_event_request_timer(acl_udp_server_event(), timer_fn,
		arg, delay, keep);
}

ACL_VSTREAM **acl_udp_server_streams()
{
	return __server ? __server->streams : NULL;
}

void acl_udp_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn, void *arg)
{
	acl_event_cancel_timer(acl_udp_server_event(), timer_fn, arg);
}

static void server_stop(SERVER *server)
{
	int i;

	for (i = 0; server->streams[i] != NULL; i++) {
		acl_event_disable_readwrite(server->event, server->streams[i]);
		acl_vstream_close(server->streams[i]);
		acl_myfree(server->streams);
	}

	acl_event_free(server->event);
}

static void servers_stop(void)
{
	int i;

	if (__servers == NULL)
		return;

	for (i = 0; i < acl_var_udp_threads; i++)
		server_stop(&__servers[i]);

	acl_myfree(__servers);
	__servers = NULL;

	acl_msg_info("All servers closed now!");
}

/* udp_server_exit - normal termination */

static void udp_server_exit(void)
{
	int i;

	if (__service_exit)
		__service_exit(__service_ctx);

	for (i = 0; __conf_str_tab[i].name != NULL; i++)
		acl_myfree(*__conf_str_tab[i].target);
	acl_app_conf_unload();

	if (acl_var_udp_procname)
		acl_myfree(acl_var_udp_procname);

	if (0)
		servers_stop();

	if (__main_event)
		acl_event_free(__main_event);

	running_status_free();

	acl_msg_close();

	exit(0);
}

/* udp_server_abort - terminate after abnormal master exit */

static void udp_server_abort(int event_type acl_unused,
	ACL_EVENT *event acl_unused, ACL_VSTREAM *stream acl_unused,
	void *context acl_unused)
{
	const char *myname = "udp_server_abort";

	acl_msg_info("%s: master disconnect -- exiting", myname);
	udp_server_exit();
}

static void udp_server_read(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream, void *context acl_unused)
{
	const char *myname = "udp_server_read";

	if (__servers == NULL) {
		acl_msg_info("%s, %s(%d): Server stoping ...",
			__FILE__, myname, __LINE__);
		return;
	}

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type: %d",
			__FILE__, myname, __LINE__, event_type);

	/* 回调用户注册的处理过程 */
	__service_main(stream, (char *) __service_name, __service_argv);

	/* 清除发生在 UDP 套接字上的临时性错误，以免事件引擎报错 */
	stream->flag = 0;

	status_increase_used();
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

#ifdef ACL_UNIX

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

#endif  /* ACL_UNIX */

	/*
	 * May need this every now and then.
	 */
#ifdef ACL_WINDOWS
	acl_var_udp_pid = _getpid();
#else
	acl_var_udp_pid = getpid();
#endif

	acl_var_udp_procname = acl_mystrdup(acl_safe_basename(procname));

	acl_var_udp_log_file = getenv("SERVICE_LOG");
	if (acl_var_udp_log_file == NULL) {
		acl_var_udp_log_file = "acl_master.log";
		acl_msg_warn("%s(%d)->%s: can't get SERVICE_LOG's env value,"
			"use %s log", __FILE__, __LINE__, myname,
			acl_var_udp_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_int64_table(__conf_int64_tab);
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

static SERVER *servers_alloc(int event_mode, int nthreads, int sock_count)
{
	SERVER *servers = (SERVER *) acl_mycalloc(nthreads, sizeof(SERVER));
	int i, j;

	for (i = 0; i < nthreads; i++) {
		servers[i].event = acl_event_new(event_mode, 0,
			acl_var_udp_delay_sec, acl_var_udp_delay_usec);

		servers[i].socket_count = sock_count;
		servers[i].streams = (ACL_VSTREAM **)
			acl_mycalloc(sock_count + 1, sizeof(ACL_VSTREAM *));
		for (j = 0; j < sock_count; j++) {
			servers[i].streams[j] = NULL;
		}
	}

	return servers;
}

static int __fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;

static void server_binding(SERVER *server, ACL_ARGV *addrs)
{
	ACL_ITER iter;
	int i = 0;

	acl_foreach(iter, addrs) {
		char addr[64];
		ACL_VSTREAM *stream;
		const char *ptr = (char *) iter.data;
		ACL_SOCKET fd = acl_udp_bind(ptr, ACL_NON_BLOCKING);

		if (fd == ACL_SOCKET_INVALID)
			continue;

		acl_msg_info("bind %s addr ok, fd %d", ptr, fd);
		stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_udp_buf_size,
			acl_var_udp_rw_timeout, __fdtype);

		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(stream, addr);
		acl_vstream_set_udp_io(stream);
		acl_event_enable_read(server->event, stream,
			0, udp_server_read, server);
#ifdef ACL_UNIX
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
#endif
		server->streams[i++] = stream;
	}

	if (i == 0)
		acl_msg_fatal("%s(%d), %s: binding all addrs failed!",
			__FILE__, __LINE__, __FUNCTION__);
}

static SERVER *servers_binding(const char *service,
	int event_mode, int nthreads)
{
	ACL_ARGV *addrs = acl_ifconf_search(service);
	SERVER   *servers;
	int       i = 0;

	if (addrs == NULL)
		acl_msg_fatal("%s(%d), %s: no addrs available for %s",
			__FILE__, __LINE__, __FUNCTION__, service);

	__socket_count = addrs->argc;
	servers = servers_alloc(event_mode, nthreads, __socket_count);

	for (i = 0; i < nthreads; i++)
		server_binding(&servers[i], addrs);

	acl_argv_free(addrs);
	return servers;
}

#ifdef ACL_UNIX

static void server_open(SERVER *server, int sock_count)
{
	ACL_SOCKET fd;
	int i = 0;

	/* socket count is as same listen_fd_count in parent process */

	fd = ACL_MASTER_LISTEN_FD;
	for (i = 0; fd < ACL_MASTER_LISTEN_FD + sock_count; fd++) {
		char addr[64];
		ACL_VSTREAM *stream = acl_vstream_fdopen(fd, O_RDWR,
			acl_var_udp_buf_size,
			acl_var_udp_rw_timeout,
			__fdtype);

		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(stream, addr);
		acl_vstream_set_udp_io(stream);
		acl_non_blocking(fd, ACL_NON_BLOCKING);
		acl_event_enable_read(server->event, stream,
			0, udp_server_read, server);
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		server->streams[i] = stream;
	}
}

static SERVER *servers_open(int event_mode, int nthreads, int sock_count)
{
	SERVER *servers;
	int i;

	servers = servers_alloc(event_mode, nthreads, sock_count);

	for (i = 0; i < nthreads; i++)
		server_open(&servers[i], sock_count);

	return servers;
}

#endif /* ACL_UNIX */

static SERVER *servers_create(const char *service, int nthreads)
{
	if (strcasecmp(acl_var_udp_event_mode, "poll") == 0)
		__event_mode = ACL_EVENT_POLL;
	else if (strcasecmp(acl_var_udp_event_mode, "kernel") == 0)
		__event_mode = ACL_EVENT_KERNEL;
	else
		__event_mode = ACL_EVENT_SELECT;

	__main_event = acl_event_new(__event_mode, 0,
		acl_var_udp_delay_sec, acl_var_udp_delay_usec);

	if (!__daemon_mode)
		return servers_binding(service, __event_mode, nthreads);

#ifdef ACL_UNIX
# ifdef SO_REUSEPORT
	return servers_binding(service, __event_mode, nthreads);
# endif
 
	/* __socket_count from command argv */
	return servers_open(__event_mode, nthreads, __socket_count);
#else
	acl_msg_fatal("%s(%d): not support daemon mode!", __FILE__, __LINE__);
	/* not reached here */
	return NULL;
#endif /* ACL_UNIX */
}

static void *thread_main(void *ctx)
{
	/* set thread local storage */
	__server = (SERVER *) ctx;

	if (__thread_init)
		__thread_init(__thread_init_ctx);

	while (1)
		acl_event_loop(__server->event);

	/* not reached here */
	return NULL;
}

/* udp_server_timeout - idle time exceeded */

static void udp_server_timeout(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context acl_unused)
{
	const char *myname = "udp_server_timeout";
	time_t now = time(NULL);
	long long last = status_last();
	long long time_left = (long long) ((acl_event_cancel_timer(event,
		udp_server_timeout, event) + 999999) / 1000000);

	if (time_left <= 0 && last + acl_var_udp_idle_limit > now)
		time_left = last + acl_var_udp_idle_limit - now;

	if (time_left > 0) {
		acl_event_request_timer(__main_event,
			udp_server_timeout,
			__main_event,
			(acl_int64) time_left * 1000000, 0);
	} else {
		acl_msg_info("%s: idle timeout -- exiting", myname);
		udp_server_exit();
	}
}

static void main_thread_loop(void)
{
#ifdef ACL_UNIX
	if (__daemon_mode) {
		ACL_VSTREAM *stat_stream = acl_vstream_fdopen(
			ACL_MASTER_STATUS_FD, O_RDWR, 8192, 0,
			ACL_VSTREAM_TYPE_SOCK);

		acl_event_enable_read(__main_event, stat_stream, 0,
			udp_server_abort, __main_event);

		acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
		acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
		acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);
	}
#endif

	if (acl_var_udp_idle_limit > 0)
		acl_event_request_timer(__main_event,
			udp_server_timeout,
			__main_event,
			(acl_int64) acl_var_udp_idle_limit * 1000000, 0);

	while (1)
		acl_event_loop(__main_event);
}

static void servers_start(SERVER *servers, int nthreads)
{
	acl_pthread_attr_t attr;
	int i;

	running_status_init();
	acl_pthread_attr_init(&attr);
	acl_pthread_attr_setdetachstate(&attr, ACL_PTHREAD_CREATE_DETACHED);

	for (i = 0; i < nthreads; i++)
		acl_pthread_create(&servers[i].tid, &attr,
			thread_main, &servers[i]);

	main_thread_loop();
}

static void usage(int argc, char *argv[])
{
	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc %d", __FILE__, __LINE__, argc);

	acl_msg_info("usage: %s -h[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -u [use setgid initgroups setuid]"
		" -v [on acl_msg_verbose]"
		" -f conf_file", argv[0]);
}

void acl_udp_server_main(int argc, char **argv, ACL_UDP_SERVER_FN service, ...)
{
	const char *myname = "acl_udp_server_main";
	const char *service_name = acl_safe_basename(argv[0]);
	ACL_MASTER_SERVER_INIT_FN pre_init = 0;
	ACL_MASTER_SERVER_INIT_FN post_init = 0;
	char   *root_dir = 0, *user_name = 0;
	const char *conf_file_ptr = 0;
	int     c, key;
	va_list ap;

	/*
	 * Pick up policy settings from master process. Shut up error messages
	 * to stderr, because no-one is going to see them.
	 */
#ifdef ACL_UNIX
	opterr = 0;
	optind = 0;
	optarg = 0;
#endif

	master_log_open(argv[0]);

	while ((c = getopt(argc, argv, "hcn:s:t:uvf:r")) > 0) {
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
			/* deprecated, just go through */
			break;
		case 'v':
			acl_msg_verbose++;
			break;
		case 'r':
			__daemon_mode = 0;
			break;
		default:
			break;
		}
	}

	udp_server_init(argv[0]);

	if (conf_file_ptr && acl_msg_verbose)
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
		case ACL_APP_CTL_THREAD_INIT:
			__thread_init =
				va_arg(ap, ACL_MASTER_SERVER_THREAD_INIT_FN);
			break;
		case ACL_APP_CTL_THREAD_INIT_CTX:
			__thread_init_ctx = va_arg(ap, void *);
			break;
		case ACL_APP_CTL_THREAD_EXIT:
			__thread_exit =
				va_arg(ap, ACL_MASTER_SERVER_THREAD_EXIT_FN);
			break;
		case ACL_APP_CTL_THREAD_EXIT_CTX:
			__thread_exit_ctx = va_arg(ap, void *);
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

	/* 设置回回调过程相关参数 */
	__service_main = service;
	__service_name = service_name;
	__service_argv = argv + optind;

	/*******************************************************************/

	if (acl_var_udp_threads <= 0)
		acl_var_udp_threads = 1;

	__servers = servers_create(service_name, acl_var_udp_threads);

	/* 必须先设置主线程的对象，以便于应用能及时使用 */
	__server = &__servers[acl_var_udp_threads - 1];

	/* 切换用户运行身份前回调应用设置的回调函数 */
	if (pre_init)
		pre_init(__service_ctx);

#ifdef ACL_UNIX
	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_udp_enable_core)
		acl_set_core_limit(0);

	/* 在切换用户运行身份前切换程序运行目录 */
	if (__daemon_mode && chdir(acl_var_udp_queue_dir) < 0)
		acl_msg_fatal("chdir(\"%s\"): %s",
			acl_var_udp_queue_dir, acl_last_serror());
#endif

#ifdef ACL_UNIX
	if (user_name)
		acl_chroot_uid(root_dir, user_name);
#endif

	udp_server_open_log();
	log_event_mode(__event_mode);

	/* 进程初始化完毕后回调此函数，以使用户可以初始化自己的环境 */
	if (post_init)
		post_init(__service_ctx);

	acl_msg_info("%s -- %s: daemon started", argv[0], myname);

	servers_start(__servers, acl_var_udp_threads);
}
