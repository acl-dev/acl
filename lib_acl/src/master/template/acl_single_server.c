/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifndef ACL_CLIENT_ONLY
#ifdef ACL_UNIX

#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
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

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_chroot_uid.h"
#include "stdlib/unix/acl_core_limit.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/unix/acl_watchdog.h"
#include "stdlib/acl_split_at.h"
#include "stdlib/unix/acl_safe_open.h"
#include "net/acl_listen.h"
#include "net/acl_tcp_ctl.h"
#include "net/acl_sane_socket.h"
#include "event/acl_events.h"

/* Application-specific */

#include "master/acl_master_flow.h"
#include "master/acl_master_proto.h"
#include "master/acl_single_params.h"
#include "master/acl_server_api.h"
#include "master/acl_master_type.h"
#include "master/acl_master_conf.h"
#include "master_log.h"

int   acl_var_single_pid;
char *acl_var_single_procname;
char *acl_var_single_log_file;

int   acl_var_single_buf_size;
int   acl_var_single_rw_timeout;
int   acl_var_single_in_flow_delay;
int   acl_var_single_idle_limit;
int   acl_var_single_delay_sec;
int   acl_var_single_delay_usec;
int   acl_var_single_daemon_timeout;
int   acl_var_single_use_limit;
int   acl_var_single_enable_core;
int   acl_var_single_disable_core_onexit;
int   acl_var_single_max_debug;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ ACL_VAR_SINGLE_BUF_SIZE, ACL_DEF_SINGLE_BUF_SIZE,
		&acl_var_single_buf_size, 0, 0 },
	{ ACL_VAR_SINGLE_RW_TIMEOUT, ACL_DEF_SINGLE_RW_TIMEOUT,
		&acl_var_single_rw_timeout, 0, 0 },
	{ ACL_VAR_SINGLE_IN_FLOW_DELAY, ACL_DEF_SINGLE_IN_FLOW_DELAY,
		&acl_var_single_in_flow_delay, 0, 0 },
	{ ACL_VAR_SINGLE_IDLE_LIMIT, ACL_DEF_SINGLE_IDLE_LIMIT,
		&acl_var_single_idle_limit, 0, 0 },
	{ ACL_VAR_SINGLE_DELAY_SEC, ACL_DEF_SINGLE_DELAY_SEC,
		&acl_var_single_delay_sec, 0, 0 },
	{ ACL_VAR_SINGLE_DELAY_USEC, ACL_DEF_SINGLE_DELAY_USEC,
		&acl_var_single_delay_usec, 0, 0 },
	{ ACL_VAR_SINGLE_DAEMON_TIMEOUT, ACL_DEF_SINGLE_DAEMON_TIMEOUT,
		&acl_var_single_daemon_timeout, 0, 0 },
	{ ACL_VAR_SINGLE_USE_LIMIT, ACL_DEF_SINGLE_USE_LIMIT,
		&acl_var_single_use_limit, 0, 0 },
	{ ACL_VAR_SINGLE_ENABLE_CORE, ACL_DEF_SINGLE_ENABLE_CORE,
		&acl_var_single_enable_core, 0, 0 },
	{ ACL_VAR_SINGLE_DISABLE_CORE_ONEXIT, ACL_DEF_SINGLE_DISABLE_CORE_ONEXIT,
		&acl_var_single_disable_core_onexit, 0, 0 },
	{ ACL_VAR_SINGLE_MAX_DEBUG, ACL_DEF_SINGLE_MAX_DEBUG,
		&acl_var_single_max_debug, 0, 0 },

	{ 0, 0, 0, 0, 0 },
};

long long int acl_var_single_core_limit;

static ACL_CONFIG_INT64_TABLE __conf_int64_tab[] = {
	{ ACL_VAR_SINGLE_CORE_LIMIT, ACL_DEF_SINGLE_CORE_LIMIT,
		&acl_var_single_core_limit, 0, 0 },
        { 0, 0, 0, 0, 0 },
};

char *acl_var_single_queue_dir;
char *acl_var_single_owner;
char *acl_var_single_pid_dir;
char *acl_var_single_log_debug;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ ACL_VAR_SINGLE_QUEUE_DIR, ACL_DEF_SINGLE_QUEUE_DIR,
		&acl_var_single_queue_dir },
	{ ACL_VAR_SINGLE_OWNER, ACL_DEF_SINGLE_OWNER,
		&acl_var_single_owner },
	{ ACL_VAR_SINGLE_PID_DIR, ACL_DEF_SINGLE_PID_DIR,
		&acl_var_single_pid_dir },
	{ ACL_VAR_SINGLE_LOG_DEBUG, ACL_DEF_SINGLE_LOG_DEBUG,
		&acl_var_single_log_debug },

	{ 0, 0, 0 },
};

 /*
  * Global state.
  */
static ACL_MASTER_SERVER_ON_LISTEN_FN __service_listen;
static ACL_SINGLE_SERVER_FN           __service_main;
static ACL_MASTER_SERVER_EXIT_FN      __service_exit;
static ACL_MASTER_SERVER_SIGHUP_FN    __sighup_handler;

static void (*__service_accept) (int, ACL_EVENT *, ACL_VSTREAM *, void *);

static int           use_count;
static ACL_EVENT    *__eventp = NULL;
static ACL_VSTREAM **__sstreams = NULL;

static char         *__service_name;
static char        **__service_argv;
static void         *__service_ctx;

static ACL_VSTREAM *__service_lock;
static int          single_server_in_flow_delay;
static unsigned     single_server_generation;

static char       __conf_file[1024];

const char *acl_single_server_conf(void)
{
	return __conf_file;
}

ACL_EVENT *acl_single_server_event()
{
	return __eventp;
}

ACL_VSTREAM **acl_single_server_sstreams()
{
	return __sstreams;
}

/* single_server_exit - normal termination */

static void single_server_exit(void)
{
#ifdef ACL_UNIX
	if (acl_var_single_disable_core_onexit)
		acl_set_core_limit(0);
#endif

	if (__service_exit)
		__service_exit(__service_ctx);

	if (__sstreams) {
		int i;

		for (i = 0; __sstreams[i] != NULL; i++)
			acl_vstream_close(__sstreams[i]);
	}

	if (__eventp)
		acl_event_free(__eventp);

	exit(0);
}

/* single_server_abort - terminate after abnormal master exit */

static void single_server_abort(int type acl_unused,
	ACL_EVENT *event acl_unused, ACL_VSTREAM *stream acl_unused,
	void *context acl_unused)
{
	if (acl_msg_verbose)
		acl_msg_info("master disconnect -- exiting");
	single_server_exit();
}

/* single_server_timeout - idle time exceeded */

static void single_server_timeout(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context acl_unused)
{
	if (acl_msg_verbose)
		acl_msg_info("idle timeout -- exiting");
	single_server_exit();
}

/* single_server_wakeup - wake up application */

static void single_server_wakeup(ACL_EVENT *event, int fd,
	const char *remote, const char *local)
{
	const char *myname = "single_server_wakeup";
	ACL_VSTREAM *stream;

	/*
	 * If the accept() succeeds, be sure to disable non-blocking I/O,
	 * because the application is supposed to be single-threaded.
	 * Notice the master of our (un)availability to service connection
	 * requests. Commit suicide when the master process disconnected
	 * from us.
	 */
	if (acl_msg_verbose)
		acl_msg_info("%s(%d), %s: connection established, fd = %d",
			__FILE__, __LINE__, myname, fd);
	acl_non_blocking(fd, ACL_BLOCKING);
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
	stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_single_buf_size,
			acl_var_single_rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	if (remote)
		acl_vstream_set_peer(stream, remote);
	if (local)
		acl_vstream_set_local(stream, local);

	if (acl_master_notify(acl_var_single_pid, single_server_generation,
		ACL_MASTER_STAT_TAKEN) < 0)
	{
		single_server_abort(ACL_EVENT_NULL_TYPE, event,
			stream, ACL_EVENT_NULL_CONTEXT);
	}

	if (single_server_in_flow_delay && acl_master_flow_get(1) < 0)
		acl_doze(acl_var_single_in_flow_delay * 1000);

	__service_main(__service_ctx, stream);

	(void) acl_vstream_fclose(stream);

	if (acl_master_notify(acl_var_single_pid, single_server_generation,
		ACL_MASTER_STAT_AVAIL) < 0) {

		single_server_abort(ACL_EVENT_NULL_TYPE, event,
			stream, ACL_EVENT_NULL_CONTEXT);
	}

	if (acl_msg_verbose)
		acl_msg_info("%s(%d), %s: connection closed, fd = %d",
			__FILE__, __LINE__, myname, fd);
	use_count++;
	if (acl_var_single_idle_limit > 0)
		acl_event_request_timer(event, single_server_timeout, NULL,
			(acl_int64) acl_var_single_idle_limit * 1000000, 0);
}

#ifdef MASTER_XPORT_NAME_PASS

/* __service_accept_pass - accept descriptor */

static void __service_accept_pass(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context acl_unused)
{
	ACL_VSTREAM *stream = (ACL_VSTREAM *) context;
	int     listen_fd = ACL_VSTREAM_SOCK(stream);
	int     time_left = -1;
	int     fd;

	/*
	 * Be prepared for accept() to fail because some other process already
	 * got the connection. We use select() + accept(), instead of simply
	 * blocking in accept(), because we must be able to detect that the
	 * master process has gone away unexpectedly.
	 */
	if (acl_var_single_idle_limit > 0)
		time_left = (int) ((acl_event_cancel_timer(event,
			single_server_timeout, NULL) + 999999) / 1000000);

	fd = PASS_ACCEPT(listen_fd);
	if (__service_lock != 0
	    && acl_myflock(ACL_VSTREAM_FILE(__service_lock),
	    	ACL_INTERNAL_LOCK, ACL_FLOCK_OP_NONE) < 0)
	{
		acl_msg_fatal("select unlock: %s", acl_last_serror());
	}
	if (fd < 0) {
		if (errno != EAGAIN)
			acl_msg_fatal("accept error: %s", acl_last_serror());
		if (time_left >= 0)
			acl_event_request_timer(event, single_server_timeout,
				NULL, (acl_int64) time_left * 1000000, 0);
	} else
		single_server_wakeup(event, fd, NULL, NULL);
}

#endif

/* __service_accept_sock - accept client connection request */

static void __service_accept_sock(int type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context acl_unused)
{
	int     listen_fd = ACL_VSTREAM_SOCK(stream);
	int     time_left = -1, fd, sock_type;
	char    remote[64], local[64];

	/*
	 * Be prepared for accept() to fail because some other process already
	 * got the connection. We use select() + accept(), instead of simply
	 * blocking in accept(), because we must be able to detect that the
	 * master process has gone away unexpectedly.
	 */
	if (acl_var_single_idle_limit > 0)
		time_left = (int) ((acl_event_cancel_timer(event,
			single_server_timeout, NULL) + 999999) / 1000000);

	fd = acl_accept(listen_fd, remote, sizeof(remote), &sock_type);
	if (__service_lock != 0
	    && acl_myflock(ACL_VSTREAM_FILE(__service_lock),
	    	ACL_INTERNAL_LOCK, ACL_FLOCK_OP_NONE) < 0)
	{
		acl_msg_fatal("select unlock: %s", acl_last_serror());
	}

#ifdef ACL_WINDOWS
	if (fd == ACL_SOCKET_INVALID) {
#else
	if (fd < 0) {
#endif
		if (errno != EAGAIN)
			acl_msg_fatal("accept error: %s", acl_last_serror());
		if (time_left > 0)
			acl_event_request_timer(event, single_server_timeout,
				NULL, (acl_int64) time_left * 1000000, 0);
		return;
	}

#ifdef AF_INET6
	if (sock_type == AF_INET || sock_type == AF_INET6)
#else
	if (sock_type == AF_INET)
#endif
		acl_tcp_set_nodelay(fd);

	if (acl_getsockname(fd, local, sizeof(local)) < 0)
		memset(local, 0, sizeof(local));

	single_server_wakeup(event, fd, remote, local);
}

static void single_server_init(const char *procname)
{
	const char *myname = "single_server_init";
	static int  inited = 0;

	if (inited)
		return;

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
	acl_var_single_pid = getpid();
	acl_var_single_procname = acl_mystrdup(acl_safe_basename(procname));

	acl_var_single_log_file = getenv("SERVICE_LOG");
	if (acl_var_single_log_file == NULL) {
		acl_var_single_log_file = acl_mystrdup("acl_master.log");
		acl_msg_fatal("%s(%d)->%s: can't get MASTER_LOG's env value,"
			" use %s log", __FILE__, __LINE__, myname,
			acl_var_single_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_int64_table(__conf_int64_tab);
	acl_get_app_conf_str_table(__conf_str_tab);

	//acl_master_vars_init(acl_var_single_buf_size, acl_var_single_rw_timeout);
}

static void single_server_open_log(const char *proc)
{
	/* first, close the master's log */
	acl_msg_info("service: %s detach acl_master's log now", proc);
	master_log_close();

	/* second, open the service's log */
	acl_msg_open(acl_var_single_log_file, acl_var_single_procname);

	if (acl_var_single_log_debug && *acl_var_single_log_debug
		&& acl_var_single_max_debug >= 100)
	{
		acl_debug_init2(acl_var_single_log_debug,
			acl_var_single_max_debug);
	}
}

static void usage(int argc, char *argv[])
{
	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc %d", __FILE__, __LINE__, argc);

	acl_msg_info("usage: %s -H[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -f conf_file", argv[0]);
}

/* acl_single_server_main - the real main program */

void acl_single_server_main(int argc, char **argv, ACL_SINGLE_SERVER_FN service,...)
{
	const char *myname = "acl_single_server_main";
	ACL_MASTER_SERVER_INIT_FN pre_init  = 0;
	ACL_MASTER_SERVER_INIT_FN post_init = 0;
	ACL_MASTER_SERVER_LOOP_FN loop      = 0;
	char *service_name = acl_mystrdup(acl_safe_basename(argv[0])), *lock_path; 
	int   c, socket_count = 1, fd, fdtype = 0, i, key;
	const char   *root_dir = 0, *user_name = 0, *transport = 0;
	const char   *generation;
	ACL_WATCHDOG *watchdog;
	ACL_VSTRING  *why;
	ACL_VSTREAM  *stream;
	ACL_VSTRING  *buf = acl_vstring_alloc(128);
	va_list ap;

	/*
	 * Pick up policy settings from master process. Shut up error messages
	 * to stderr, because no-one is going to see them.
	 */
#ifdef ACL_LINUX
	opterr = 0;
	optind = 0;
	optarg = 0;
#endif

	__conf_file[0] = 0;

	while ((c = getopt(argc, argv, "Hcn:s:t:uf:")) > 0) {
		switch (c) {
		case 'H':
			usage(argc, argv);
			exit (0);
		case 'f':
			acl_app_conf_load(optarg);
			snprintf(__conf_file, sizeof(__conf_file), "%s", optarg);
			break;
		case 'c':
			root_dir = "setme";
			break;
		case 'n':
			service_name = optarg;
			break;
		case 's':
			if ((socket_count = atoi(optarg)) > 0)
				break;
			acl_msg_fatal("invalid socket_count: %s", optarg);
			/* NOT REACHED */
			break;
		case 'u':
			user_name = "setme";
			break;
		case 't':
			transport = optarg;
			break;
		default:
			break;
		}
	}

	single_server_init(argv[0]);

	/* open log of the current service process */
	master_log_open(argv[0]);

	acl_msg_info("%s(%d): daemon started, log=%s",
		acl_var_single_procname, __LINE__, acl_var_single_log_file);

	if (__conf_file[0] == 0)
		acl_msg_fatal("%s(%d), %s: need \"-f pathname\"",
			__FILE__, __LINE__, myname);
	else if (acl_msg_verbose)
		acl_msg_info("%s(%d), %s: configure file = %s",
			__FILE__, __LINE__, myname, __conf_file);

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

		case ACL_MASTER_SERVER_ON_LISTEN:
			__service_listen =
				va_arg(ap, ACL_MASTER_SERVER_ON_LISTEN_FN);
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
		case ACL_MASTER_SERVER_LOOP:
			loop = va_arg(ap, ACL_MASTER_SERVER_LOOP_FN);
			break;
		case ACL_MASTER_SERVER_EXIT:
			__service_exit = va_arg(ap, ACL_MASTER_SERVER_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_IN_FLOW_DELAY:
			single_server_in_flow_delay = 1;
			break;
		case ACL_MASTER_SERVER_SIGHUP:
			__sighup_handler =
				va_arg(ap, ACL_MASTER_SERVER_SIGHUP_FN);
			break;
		default:
			acl_msg_panic("%s: unknown argument: %d", myname, key);
		}
	}

	va_end(ap);

	if (root_dir)
		root_dir = acl_var_single_queue_dir;
	if (user_name)
		user_name = acl_var_single_owner;

	/* If not connected to stdin, stdin must not be a terminal */
	if (isatty(STDIN_FILENO))
		acl_msg_fatal("%s(%d), %s: do not run this command by hand",
			__FILE__, __LINE__, myname);

	/* Can options be required? */
	if (transport == 0)
		acl_msg_fatal("no transport type specified");
	if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_INET) == 0) {
		__service_accept = __service_accept_sock;
		fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_UNIX) == 0) {
		__service_accept = __service_accept_sock;
		fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_UNIX;
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_SOCK) == 0) {
		__service_accept = __service_accept_sock;
		fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
#ifdef MASTER_XPORT_NAME_PASS
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_PASS) == 0) {
		__service_accept = __service_accept_pass;
		fdtype = ACL_VSTREAM_TYPE_LISTEN;
#endif
	} else
		acl_msg_fatal("unsupported transport: %s", transport);

	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &single_server_generation);
	}

	/*
	 * Traditionally, BSD select() can't handle multiple processes
	 * selecting on the same socket, and wakes up every process in
	 * select(). See TCP/IP Illustrated volume 2 page 532. We avoid
	 * select() collisions with an external lock file.
	 */
	lock_path = acl_concatenate(acl_var_single_pid_dir, "/",
			transport, ".", service_name, (char *) 0);
	why = acl_vstring_alloc(1);
	__service_lock = acl_safe_open(lock_path, O_CREAT | O_RDWR, 0600,
		(struct stat *) 0, (uid_t)-1, (uid_t )-1, why);

	if (__service_lock == 0)
		acl_msg_fatal("open lock file %s: %s",
			lock_path, acl_vstring_str(why));

	acl_close_on_exec(ACL_VSTREAM_FILE(__service_lock), ACL_CLOSE_ON_EXEC);
	acl_myfree(lock_path);
	acl_vstring_free(why);

	/* Set up call-back info. */
	__service_main = service;
	__service_name = service_name;
	__service_argv = argv + optind;

	__eventp = acl_event_new_select(acl_var_single_delay_sec,
			acl_var_single_delay_usec);

	/* Run pre-jail initialization. */

	if (chdir(acl_var_single_queue_dir) < 0)
		acl_msg_fatal("%s(%d), %s: chdir(\"%s\"): %s", __FILE__,
			__LINE__, myname, acl_var_single_queue_dir,
			acl_last_serror());

	if (pre_init)
		pre_init(__service_ctx);

#ifdef SNAPSHOT
	tzset();
#endif
	acl_chroot_uid(root_dir, user_name);

#ifdef ACL_UNIX
	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_single_enable_core && acl_var_single_core_limit != 0) {
		acl_set_core_limit(acl_var_single_core_limit);
	}
#endif

	single_server_open_log(argv[0]);

	/*
	 * Running as a semi-resident server. Service connection requests.
	 * Terminate when we have serviced a sufficient number of clients,
	 * when no-one has been talking to us for a configurable amount of
	 * time, or when the master process terminated abnormally.
	 */
	if (acl_var_single_idle_limit > 0)
		acl_event_request_timer(__eventp, single_server_timeout, NULL,
			(acl_int64) acl_var_single_idle_limit * 1000000, 0);

	__sstreams = (ACL_VSTREAM **) acl_mycalloc(
		ACL_MASTER_LISTEN_FD + socket_count, sizeof(ACL_VSTREAM *));

	i = 0;
	fd = ACL_MASTER_LISTEN_FD;
	for (; fd < ACL_MASTER_LISTEN_FD + socket_count; fd++) {
		stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_single_buf_size,
				acl_var_single_rw_timeout, fdtype);
		if (__service_listen)
			__service_listen(__service_ctx, stream);
		__sstreams[i++] = stream;

		acl_event_enable_listen(__eventp, stream, 0,
			__service_accept, stream);
		acl_close_on_exec(ACL_VSTREAM_SOCK(stream), ACL_CLOSE_ON_EXEC);
	}

	stream = acl_vstream_fdopen(ACL_MASTER_STATUS_FD,
		O_RDWR, 8192, 0, ACL_VSTREAM_TYPE_SOCK);
	acl_event_enable_read(__eventp, stream, 0, single_server_abort, NULL);

	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	watchdog = acl_watchdog_create(acl_var_single_daemon_timeout,
		(ACL_WATCHDOG_FN) 0, NULL);

	/* Run post-jail initialization. */
	if (post_init)
		post_init(__service_ctx);

#ifdef ACL_LINUX
	/* notify master that child started ok */
	if (generation)
		acl_master_notify(acl_var_single_pid, single_server_generation,
			ACL_MASTER_STAT_START_OK);
#endif

	acl_server_sighup_setup();
	acl_server_sigterm_setup();

	/* The event loop, at last. */
	while (acl_var_single_use_limit == 0 ||
		use_count < acl_var_single_use_limit) {

		if (__service_lock != 0) {
			acl_watchdog_stop(watchdog);
			if (acl_myflock(ACL_VSTREAM_FILE(__service_lock),
				ACL_INTERNAL_LOCK, ACL_FLOCK_OP_EXCLUSIVE) < 0)
			{
				acl_msg_fatal("lock %s", acl_last_serror());
			}
		}

		acl_watchdog_start(watchdog);

		if (loop != NULL) {
			int delay_sec = loop(__service_ctx);
			acl_event_set_delay_sec(__eventp, delay_sec);
		}

		acl_event_loop(__eventp);

		if (acl_var_server_gotsighup && __sighup_handler) {
			acl_var_server_gotsighup = 0;
			if (__sighup_handler(__service_ctx, buf) < 0)
				acl_master_notify(acl_var_single_pid,
					single_server_generation,
					ACL_MASTER_STAT_SIGHUP_ERR);
			else
				acl_master_notify(acl_var_single_pid,
					single_server_generation,
					ACL_MASTER_STAT_SIGHUP_OK);
		}
	}

	acl_vstring_free(buf);
}

#endif /* ACL_UNIX */
#endif /* ACL_CLIENT_ONLY */
