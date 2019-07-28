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

#include "init/acl_init.h"
#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_chroot_uid.h"
#include "stdlib/unix/acl_core_limit.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/acl_argv.h"
#include "stdlib/acl_atomic.h"
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
#include "ifmonitor.h"

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
int   acl_var_udp_disable_core_onexit;
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
	{ ACL_VAR_UDP_DISABLE_CORE_ONEXIT, ACL_DEF_UDP_DISABLE_CORE_ONEXIT,
		&acl_var_udp_disable_core_onexit, 0, 0 },
	{ ACL_VAR_UDP_MAX_DEBUG, ACL_DEF_UDP_MAX_DEBUG,
		&acl_var_udp_max_debug, 0, 0 },
	{ ACL_VAR_UDP_THREADS, ACL_DEF_UDP_THREADS,
		&acl_var_udp_threads, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

long long int acl_var_udp_use_limit;
long long int acl_var_udp_core_limit;

static ACL_CONFIG_INT64_TABLE __conf_int64_tab[] = {
	{ ACL_VAR_UDP_USE_LIMIT, ACL_DEF_UDP_USE_LIMIT,
		&acl_var_udp_use_limit, 0, 0 },
	{ ACL_VAR_UDP_CORE_LIMIT, ACL_DEF_UDP_CORE_LIMIT,
		&acl_var_udp_core_limit, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

int   acl_var_udp_threads_detached;
int   acl_var_udp_non_block;
int   acl_var_udp_fatal_on_bind_error;

static ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
	{ ACL_VAR_UDP_THREADS_DETACHED, ACL_DEF_UDP_THREADS_DETACHED,
		&acl_var_udp_threads_detached },
	{ ACL_VAR_UDP_NON_BLOCK, ACL_DEF_UDP_NON_BLOCK,
		&acl_var_udp_non_block },
	{ ACL_VAR_UDP_FATAL_ON_BIND_ERROR, ACL_DEF_UDP_FATAL_ON_BIND_ERROR,
		&acl_var_udp_fatal_on_bind_error },

	{ 0, 0, 0 },
};

char *acl_var_udp_queue_dir;
char *acl_var_udp_owner;
char *acl_var_udp_pid_dir;
char *acl_var_udp_event_mode;
char *acl_var_udp_log_debug;
char *acl_var_udp_private;
char *acl_var_udp_reuse_port;
static int var_udp_reuse_port = 0;

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
	{ ACL_VAR_UDP_PRIVATE, ACL_DEF_UDP_PRIVATE,
		&acl_var_udp_private },
	{ ACL_VAR_UDP_REUSEPORT, ACL_DEF_UDP_REUSEPORT,
		&acl_var_udp_reuse_port},

        { 0, 0, 0 },
};

typedef struct UDP_SERVER {
	acl_pthread_t tid;
	ACL_EVENT    *event;
	ACL_VSTREAM **streams;
	int           count;
	int           size;
} UDP_SERVER;

 /*
  * Global state.
  */
static ACL_UDP_SERVER_FN                __service_main;
static ACL_MASTER_SERVER_EXIT_FN        __service_exit;
static ACL_MASTER_SERVER_THREAD_INIT_FN __thread_init;
static ACL_MASTER_SERVER_SIGHUP_FN      __sighup_handler;
static ACL_MASTER_SERVER_ON_BIND_FN	__server_on_bind;
static ACL_MASTER_SERVER_ON_UNBIND_FN	__server_on_unbind;

static void *__thread_init_ctx = NULL;

static acl_pthread_key_t __server_key;

static int   __event_mode;
static int   __socket_count = 1;

static UDP_SERVER *__servers = NULL;
static int         __nservers = 0;
static ACL_EVENT  *__main_event = NULL;

static const char *__service_name;
static char      **__service_argv;
static void       *__service_ctx;
static int         __daemon_mode = 1;
static char        __conf_file[1024];
static unsigned    __udp_server_generation;
static int         __service_exiting = 0;
static long long   __servers_count = 0;
static ACL_ATOMIC *__servers_count_atomic = NULL;

#define SOCK		ACL_VSTREAM_SOCK
#define MAX	1024

/* forward functions */

static ACL_VSTREAM *server_bind_one(const char *addr);
static void udp_server_read(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context);

static void get_addr(const char *addr, char *buf, size_t size)
{
#ifdef ACL_WINDOWS
	snprintf(buf, size, "%s", addr);
#else
	if (strrchr(addr, ':') || strrchr(addr, ACL_ADDR_SEP) || acl_alldig(addr)) {
		snprintf(buf, size, "%s", addr);
	} else {
		snprintf(buf, size, "%s/%s/%s", acl_var_udp_queue_dir,
			strcasecmp(acl_var_udp_private, "n") == 0 ?
			  ACL_MASTER_CLASS_PUBLIC : ACL_MASTER_CLASS_PRIVATE,
			 addr);
	}
#endif
}

#if defined(ACL_LINUX) && defined(SO_REUSEPORT)

static void remove_stream(UDP_SERVER *server, const char *addr)
{
	int i;

	for (i = 0; i < server->count; i++) {
		char local[256];
		ACL_VSTREAM *stream = server->streams[i];

		if (acl_getsockname(SOCK(stream), local, sizeof(local)) == -1) {
			acl_msg_error("%s(%d): getsockname error %s",
				__FUNCTION__, __LINE__, acl_last_serror());
			continue;
		}

		if (strcmp(local, addr) != 0) {
			continue;
		}

		if (__server_on_unbind) {
			__server_on_unbind(__service_ctx, stream);
		}

		acl_event_disable_readwrite(server->event, stream);
		acl_vstream_close(stream);

		--server->count;
		assert(server->count >= 0);

		if (server->count == 0) {
			server->streams[0] = NULL;
		} else {
			server->streams[i] = server->streams[server->count];
		}

		acl_msg_info("remove one stream ok, addr=%s", addr);
		break;
	}
}

static void server_del_addrs(UDP_SERVER *server, ACL_ARGV *addrs)
{
	ACL_ITER iter;

	acl_assert(addrs->argc > 0); 

	acl_foreach(iter, addrs) {
		const char *addr = (const char *) iter.data;
		remove_stream(server, addr);
	}
}

static void server_add_addrs(UDP_SERVER *server, ACL_HTABLE *addrs)
{
	ACL_ITER iter;
	int  size = acl_htable_used(addrs);

	acl_assert(size > 0);

	if (server->count + size >= server->size) {
		server->size    = server->count + size + 1;
		server->streams = (ACL_VSTREAM **) acl_myrealloc(
			server->streams, server->size * sizeof(ACL_VSTREAM *));
	}

	acl_foreach(iter, addrs) {
		const char *ptr = (const char *) iter.key;
		ACL_VSTREAM *stream = server_bind_one(ptr);

		if (stream == NULL) {
			acl_msg_error("%s(%d): bind %s error %s", __FUNCTION__,
				__LINE__, ptr, acl_last_serror());
			continue;
		}

		if (__server_on_bind) {
			__server_on_bind(__service_ctx, stream);
		}

		server->streams[server->count++] = stream;
		acl_event_enable_read(server->event, stream, 0,
			udp_server_read, server);

		acl_msg_info("bind %s addr ok, fd %d",
			ACL_VSTREAM_LOCAL(stream), SOCK(stream));
	}
}

static void server_rebinding(UDP_SERVER *server, ACL_HTABLE *addrs2add)
{
	ACL_ARGV *addrs2del = acl_argv_alloc(10);
	int i;

	for (i = 0; i < server->count; i++) {
		char addr[MAX];
		ACL_VSTREAM *stream = server->streams[i];

		if (acl_getsockname(SOCK(stream), addr, sizeof(addr)) == -1) {
			acl_msg_error("%s(%d): getsockname error %s",
				__FUNCTION__, __LINE__, acl_last_serror());
			continue;
		}

		/**
		 * The table holds all the NICS's addrs from searching, if the
		 * current stream's addr is in the table, just delete it from
		 * the table which the addrs left in table will be as the new
		 * addrs to be bound; If the stream's addr isn't in the table,
		 * then the stream with the addr will be closed.
		 */
		if (acl_htable_locate(addrs2add, addr) != NULL) {
			acl_htable_delete(addrs2add, addr, NULL);
		} else {
			acl_argv_add(addrs2del, addr, NULL);
		}
	}

	if (acl_argv_size(addrs2del) > 0) {
		server_del_addrs(server, addrs2del);
	}

	if (acl_htable_used(addrs2add) > 0) {
		server_add_addrs(server, addrs2add);
	}

	acl_argv_free(addrs2del);
}

static void netlink_on_changed(void *ctx)
{
	UDP_SERVER *server    = (UDP_SERVER *) ctx;
	ACL_IFCONF *ifconf    = acl_ifconf_search(__service_name);
	ACL_HTABLE *addrs2add = acl_htable_create(10, 0);
	ACL_ITER    iter;
	char        addr[MAX];

	if (ifconf == NULL) {
		acl_msg_error("%s(%d): acl_ifconf_search null, service=%s",
			__FUNCTION__, __LINE__, __service_name);
		acl_htable_free(addrs2add, NULL);
		return;
	}

	acl_foreach(iter, ifconf) {
		const ACL_IFADDR *ifaddr = (const ACL_IFADDR *) iter.data;
		get_addr(ifaddr->addr, addr, sizeof(addr));
		acl_htable_enter(addrs2add, addr, addr);
	}

	server_rebinding(server, addrs2add);
	acl_htable_free(addrs2add, NULL);
	acl_free_ifaddrs(ifconf);
}

#endif	// ACL_LINUX && SO_REUSEPORT

const char *acl_udp_server_conf(void)
{
	return __conf_file;
}

static ACL_ATOMIC_CLOCK *__clock = NULL;

ACL_EVENT *acl_udp_server_event(void)
{
	if ((unsigned long long) acl_pthread_self() == acl_main_thread_self()) {
		acl_assert(__main_event);
		return __main_event;
	} else {
		UDP_SERVER *server = (UDP_SERVER *) acl_pthread_getspecific(__server_key);
		acl_assert(server);
		return server->event;
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
	UDP_SERVER *server = (UDP_SERVER *) acl_pthread_getspecific(__server_key);
	acl_assert(server);
	return server ? server->streams : NULL;
}

void acl_udp_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn, void *arg)
{
	acl_event_cancel_timer(acl_udp_server_event(), timer_fn, arg);
}

/*
static void server_stop(UDP_SERVER *server)
{
	int i;

	for (i = 0; server->streams[i] != NULL; i++) {
		acl_event_disable_readwrite(server->event, server->streams[i]);
		acl_vstream_close(server->streams[i]);
		if (server->ipc_in)
			acl_vstream_close(server->ipc_in);
		if (server->ipc_out)
			acl_vstream_close(server->ipc_out);
		if (server->lock) {
			acl_pthread_mutex_destroy(server->lock);
			acl_myfree(server->lock);
		}
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
*/

/* udp_server_exit - normal termination */

static void udp_server_exit(void)
{
	int i;
	long long n = 0;

	if (__service_exiting) {
		return;
	}

#ifdef ACL_UNIX
	if (acl_var_udp_disable_core_onexit) {
		acl_set_core_limit(0);
	}
#endif

	__service_exiting = 1;

	if (__service_exit)
		__service_exit(__service_ctx);

	for (i = 0; i < 10; i++) {
		n = acl_atomic_int64_fetch_add(__servers_count_atomic, 0);
		if (n <= 0) {
			break;
		}
		acl_msg_info("%s(%d): waiting running threads=%lld, %lld",
			__FUNCTION__, __LINE__, n, __servers_count);
		sleep(1);
	}

	acl_msg_info("%s(%d): all threads exit -- %s, left=%lld, %lld",
		__FUNCTION__, __LINE__, __servers_count > 0 ? "no" : "yes",
		n, __servers_count);

	for (i = 0; __conf_str_tab[i].name != NULL; i++) {
		acl_myfree(*__conf_str_tab[i].target);
	}

	acl_app_conf_unload();

	if (acl_var_udp_procname) {
		acl_myfree(acl_var_udp_procname);
	}

//	if (0)
//		servers_stop();

	if (__main_event)
		acl_event_free(__main_event);

	acl_atomic_free(__servers_count_atomic);
	__servers_count_atomic = NULL;

	acl_atomic_clock_free(__clock);
	__clock = NULL;

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

	if (event_type != ACL_EVENT_READ) {
		acl_msg_fatal("%s, %s(%d): unknown event_type: %d",
			__FILE__, myname, __LINE__, event_type);
	}

	/* 回调用户注册的处理过程 */
	__service_main(__service_ctx, stream);

	/* 清除发生在 UDP 套接字上的临时性错误，以免事件引擎报错 */
	stream->flag = 0;

	acl_atomic_clock_count_add(__clock, 1);
}

static void udp_server_init(const char *procname)
{
	const char *myname = "udp_server_init";
	static int inited = 0;

	if (inited)
		return;

	inited = 1;

	if (procname == NULL || *procname == 0) {
		acl_msg_fatal("%s(%d); procname null", myname, __LINE__);
	}

#ifdef ACL_UNIX

	/* Don't die when a process goes away unexpectedly. */
	signal(SIGPIPE, SIG_IGN);

	/* Don't die for frivolous reasons. */
#ifdef SIGXFSZ
	signal(SIGXFSZ, SIG_IGN);
#endif

#endif  /* ACL_UNIX */

	/* May need this every now and then. */
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
	acl_get_app_conf_bool_table(__conf_bool_tab);

	if (strcasecmp(acl_var_udp_reuse_port, "y") == 0
		|| strcasecmp(acl_var_udp_reuse_port, "yes") == 0
		|| strcasecmp(acl_var_udp_reuse_port, "true") == 0
		|| strcasecmp(acl_var_udp_reuse_port, "on") == 0) {

		var_udp_reuse_port = 1;
	}
}

static void udp_server_open_log(void)
{
	/* first, close the master's log */
	master_log_close();

	/* second, open the service's log */
	acl_msg_open(acl_var_udp_log_file, acl_var_udp_procname);

	if (acl_var_udp_log_debug && *acl_var_udp_log_debug
		&& acl_var_udp_max_debug >= 100) {

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

static UDP_SERVER *servers_alloc(int event_mode, int nthreads, int sock_count)
{
	UDP_SERVER *servers = (UDP_SERVER *)
		acl_mycalloc(nthreads, sizeof(UDP_SERVER));
	int i;

	for (i = 0; i < nthreads; i++) {
		servers[i].event = acl_event_new(event_mode, 0,
			acl_var_udp_delay_sec, acl_var_udp_delay_usec);

		servers[i].count   = sock_count;
		servers[i].size    = sock_count;
		servers[i].streams = (ACL_VSTREAM **)
			acl_mycalloc(sock_count + 1, sizeof(ACL_VSTREAM *));
	}

	return servers;
}

static int __fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;

static ACL_VSTREAM *server_bind_one(const char *addr)
{
	ACL_VSTREAM *stream;
	ACL_SOCKET   fd;
	unsigned flag = 0;
	char     local[MAX];

	if (acl_var_udp_non_block) {
		flag |= ACL_INET_FLAG_NBLOCK;
	}

	if (var_udp_reuse_port) {
		flag |= ACL_INET_FLAG_REUSEPORT;
	}

	fd = acl_udp_bind(addr, flag);
	if (fd == ACL_SOCKET_INVALID) {
		acl_msg_warn("bind %s error %s", addr, acl_last_serror());
		return NULL;
	}

#ifdef ACL_UNIX
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
#endif

	stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_udp_buf_size,
			acl_var_udp_rw_timeout, __fdtype);

	acl_getsockname(fd, local, sizeof(local));
	acl_vstream_set_local(stream, local);
	acl_vstream_set_udp_io(stream);

	return stream;
}

static void server_binding(UDP_SERVER *server, ACL_IFCONF *ifconf)
{
	char addr[MAX];
	ACL_ITER iter;
	int i = 0;

	acl_foreach(iter, ifconf) {
		ACL_VSTREAM *stream;
		ACL_IFADDR *ifaddr = (ACL_IFADDR *) iter.data;

		get_addr(ifaddr->addr, addr, sizeof(addr));
		stream = server_bind_one(addr);
		if (stream == NULL) {
			if (acl_var_udp_fatal_on_bind_error) {
				acl_msg_fatal("%s(%d): bind %s error %s",
					__FUNCTION__, __LINE__, addr,
					acl_last_serror());
			}

			acl_msg_error("%s(%d): bind %s error %s", __FUNCTION__,
				__LINE__, addr, acl_last_serror());
			continue;
		}

		acl_event_enable_read(server->event, stream,
			0, udp_server_read, server);
		server->streams[i++] = stream;
		acl_msg_info("%s(%d), %s: bind %s addr ok, fd %d",
			__FILE__, __LINE__, __FUNCTION__,
			(char *) iter.data, SOCK(stream));
	}

	if (i == 0) {
		acl_msg_fatal("%s(%d), %s: binding all addrs failed!",
			__FILE__, __LINE__, __FUNCTION__);
	}

	server->count = i;
}

static UDP_SERVER *servers_binding(const char *service,
	int event_mode, int nthreads)
{
	ACL_IFCONF *ifconf = acl_ifconf_search(service);
	UDP_SERVER *servers;
	int i = 0;

	if (ifconf == NULL) {
		acl_msg_fatal("%s(%d), %s: no addrs available for %s",
			__FILE__, __LINE__, __FUNCTION__, service);
	}

	__socket_count = ifconf->length;
	servers = servers_alloc(event_mode, nthreads, __socket_count);

	for (i = 0; i < nthreads; i++) {
		server_binding(&servers[i], ifconf);
	}

	acl_free_ifaddrs(ifconf);
	return servers;
}

#ifdef ACL_UNIX

static void server_open(UDP_SERVER *server, int sock_count)
{
	ACL_SOCKET fd = ACL_MASTER_LISTEN_FD;
	int i = 0;

	/* socket count is as same listen_fd_count in parent process */

	for (; fd < ACL_MASTER_LISTEN_FD + sock_count; fd++) {
		char addr[64];
		ACL_VSTREAM *stream = acl_vstream_fdopen(fd, O_RDWR,
			acl_var_udp_buf_size,
			acl_var_udp_rw_timeout,
			__fdtype);

		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(stream, addr);
		acl_vstream_set_udp_io(stream);

		acl_msg_info("%s(%d), %s: addr=%s, fd=%d, sock_count=%d",
			__FILE__, __LINE__, __FUNCTION__, addr, fd, sock_count);
		server->streams[i++] = stream;
		acl_non_blocking(fd, ACL_NON_BLOCKING);
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

		acl_event_enable_read(server->event, stream,
			0, udp_server_read, server);
	}
}

static UDP_SERVER *servers_open(int event_mode, int nthreads, int sock_count)
{
	UDP_SERVER *servers;
	int i;

	servers = servers_alloc(event_mode, nthreads, sock_count);

	for (i = 0; i < nthreads; i++) {
		server_open(&servers[i], sock_count);
	}

	return servers;
}

#endif

static UDP_SERVER *servers_create(const char *service, int nthreads)
{
	if (strcasecmp(acl_var_udp_event_mode, "poll") == 0) {
		__event_mode = ACL_EVENT_POLL;
	} else if (strcasecmp(acl_var_udp_event_mode, "kernel") == 0) {
		__event_mode = ACL_EVENT_KERNEL;
	} else {
		__event_mode = ACL_EVENT_SELECT;
	}

	__main_event = acl_event_new(__event_mode, 0,
		acl_var_udp_delay_sec, acl_var_udp_delay_usec);

	if (!__daemon_mode) {
		return servers_binding(service, __event_mode, nthreads);
	}


#ifdef	ACL_UNIX
# ifdef SO_REUSEPORT
	if (var_udp_reuse_port) {
		return servers_binding(service, __event_mode, nthreads);
	} else {
		return servers_open(__event_mode, nthreads, __socket_count);
	}
# else
	/* __socket_count from command argv */
	return servers_open(__event_mode, nthreads, __socket_count);
# endif
#else
	acl_msg_fatal("%s(%d): not support daemon mode!", __FILE__, __LINE__);
	return NULL;  /* just avoid compiling warning */
#endif
}

static void *thread_main(void *ctx)
{
	/* set thread local storage */
	UDP_SERVER *server = (UDP_SERVER *) ctx;
	acl_pthread_setspecific(__server_key, server);

	if (__thread_init) {
		__thread_init(__thread_init_ctx);
	}

	acl_atomic_int64_add_fetch(__servers_count_atomic, 1);
	acl_msg_info("%s(%d), %s: current threads count=%lld",
		__FILE__, __LINE__, __FUNCTION__, __servers_count);

#if defined(ACL_LINUX) && defined(SO_REUSEPORT)
	if (var_udp_reuse_port) {
		netlink_monitor(server->event, netlink_on_changed, server);
	}
#endif

	while (!__service_exiting) {
		acl_event_loop(server->event);
	}

	acl_msg_info("%s(%d), %s: thread-%lu exit", __FILE__, __LINE__,
		__FUNCTION__, (unsigned long) acl_pthread_self());

	if (__servers_count_atomic && __servers_count > 0) {
		acl_atomic_int64_add_fetch(__servers_count_atomic, -1);
	}
	return NULL;
}

/* udp_server_timeout - idle time exceeded */

static void udp_server_timeout(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context acl_unused)
{
	const char *myname = "udp_server_timeout";
	time_t now = time(NULL);
	long long last = acl_atomic_clock_atime(__clock) / 1000000;
	long long time_left = (long long) ((acl_event_cancel_timer(event,
		udp_server_timeout, event) + 999999) / 1000000);

	if (time_left <= 0 && last + acl_var_udp_idle_limit > now) {
		time_left = last + acl_var_udp_idle_limit - now;
	}

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
	ACL_VSTRING *buf = acl_vstring_alloc(128);

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

	if (acl_var_udp_idle_limit > 0) {
		acl_event_request_timer(__main_event, udp_server_timeout,
			__main_event,
			(acl_int64) acl_var_udp_idle_limit * 1000000, 0);
	}

	while (!__service_exiting) {
		acl_event_loop(__main_event);
#ifdef ACL_UNIX
		if (!acl_var_server_gotsighup || !__sighup_handler) {
			continue;
		}

		acl_var_server_gotsighup = 0;
		ACL_VSTRING_RESET(buf);
		if (__sighup_handler(__service_ctx, buf) < 0) {
			acl_master_notify(acl_var_udp_pid,
				__udp_server_generation,
				ACL_MASTER_STAT_SIGHUP_ERR);
		} else {
			acl_master_notify(acl_var_udp_pid,
				__udp_server_generation,
				ACL_MASTER_STAT_SIGHUP_OK);
		}
#endif
	}

	acl_vstring_free(buf);
	acl_msg_info("%s(%d): main thread-%lu exit",
		__FUNCTION__, __LINE__, (unsigned long) acl_pthread_self());
}

static void servers_start(UDP_SERVER *servers, int nthreads)
{
	acl_pthread_attr_t attr;
	int i;

	if (nthreads <= 0) {
		acl_msg_fatal("%s(%d), %s: invalid nthreads %d",
			__FILE__, __LINE__, __FUNCTION__, nthreads);
	}

	if (__server_on_bind) {
		for (i = 0; i < nthreads; i++) {
			UDP_SERVER *server = &servers[i];
			int j;

			for (j = 0; j < server->count; j++) {
				__server_on_bind(__service_ctx,
					server->streams[j]);
			}
		}
	}

	__servers_count_atomic = acl_atomic_new();
	acl_atomic_set(__servers_count_atomic, &__servers_count);
	acl_atomic_int64_set(__servers_count_atomic, 0);

	__clock = acl_atomic_clock_alloc();

	acl_pthread_attr_init(&attr);
	if (acl_var_udp_threads_detached) {
		acl_pthread_attr_setdetachstate(&attr,
			ACL_PTHREAD_CREATE_DETACHED);
	}

	for (i = 0; i < nthreads; i++) {
		acl_pthread_create(&servers[i].tid, &attr,
			thread_main, &servers[i]);
	}

	main_thread_loop();
}

static void thread_server_exit(void *ctx acl_unused)
{
	acl_msg_info("%s(%d), %s: thread-%lu exit now", __FILE__, __LINE__,
		__FUNCTION__, (unsigned long) acl_pthread_self());

	if (__servers_count_atomic && __servers_count > 0) {
		acl_atomic_int64_add_fetch(__servers_count_atomic, -1);
	}
}

static void usage(int argc, char *argv[])
{
	if (argc <= 0) {
		acl_msg_fatal("%s(%d): argc %d", __FILE__, __LINE__, argc);
	}

	acl_msg_info("usage: %s -H[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -u [use setgid initgroups setuid]"
		" -f conf_file", argv[0]);
}

void acl_udp_server_main(int argc, char **argv, ACL_UDP_SERVER_FN service, ...)
{
	const char *myname = "acl_udp_server_main";
	const char *service_name = acl_safe_basename(argv[0]);
	ACL_MASTER_SERVER_INIT_FN pre_init = 0;
	ACL_MASTER_SERVER_INIT_FN post_init = 0;
	char   *root_dir = 0, *user_name = 0;
	UDP_SERVER *server;
#ifdef ACL_UNIX
	const char *generation;
#endif
	int     c, key;
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
	master_log_open(argv[0]);

	while ((c = getopt(argc, argv, "Hcn:s:t:uf:r")) > 0) {
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
			if ((__socket_count = atoi(optarg)) > 0)
				break;
			acl_msg_fatal("invalid socket_count: %s", optarg);
			/* NOT REACHED */
			break;
		case 'u':
			user_name = "setme";
			break;
		case 't':
			/* deprecated, just go through */
			break;
		case 'r':
			__daemon_mode = 0;
			break;
		default:
			break;
		}
	}

	udp_server_init(argv[0]);

	if (__conf_file[0] && acl_msg_verbose) {
		acl_msg_info("%s(%d), %s: configure file=%s", 
			__FILE__, __LINE__, myname, __conf_file);
	}

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
		case ACL_MASTER_SERVER_SIGHUP:
			__sighup_handler =
				va_arg(ap, ACL_MASTER_SERVER_SIGHUP_FN);
			break;
		case ACL_MASTER_SERVER_ON_BIND:
			__server_on_bind =
				va_arg(ap, ACL_MASTER_SERVER_ON_BIND_FN);
			break;
		case ACL_MASTER_SERVER_ON_UNBIND:
			__server_on_unbind =
				va_arg(ap, ACL_MASTER_SERVER_ON_UNBIND_FN);
			break;
		default:
			acl_msg_panic("%s: unknown type: %d", myname, key);
		}
	}

	va_end(ap);

	if (root_dir) {
		root_dir = acl_var_udp_queue_dir;
	}

	if (user_name) {
		user_name = acl_var_udp_owner;
	}

	/* 设置回回调过程相关参数 */
	__service_main = service;
	__service_name = service_name;
	__service_argv = argv + optind;

#ifdef ACL_UNIX
	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation)) {
			acl_msg_fatal("bad generation: %s", generation);
		}
		sscanf(generation, "%o", &__udp_server_generation);
	}
#endif

	/*******************************************************************/

	if (acl_var_udp_threads <= 0) {
		acl_var_udp_threads = 1;
	}

	__servers = servers_create(service_name, acl_var_udp_threads);
	if (__servers) {
		__nservers = acl_var_udp_threads;
	}

	/* 创建用于线程局部变量的键对象 */
	acl_pthread_key_create(&__server_key, thread_server_exit);

	/* 必须先设置主线程的对象，以便于应用能及时使用 */
	server = &__servers[acl_var_udp_threads - 1];
	acl_pthread_setspecific(__server_key, server);

	/* 切换用户运行身份前回调应用设置的回调函数 */
	if (pre_init) {
		pre_init(__service_ctx);
	}

#ifdef ACL_UNIX
	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_udp_enable_core && acl_var_udp_core_limit != 0) {
		acl_set_core_limit(acl_var_udp_core_limit);
	}

	/* 在切换用户运行身份前切换程序运行目录 */
	if (__daemon_mode && chdir(acl_var_udp_queue_dir) < 0) {
		acl_msg_fatal("chdir(\"%s\"): %s",
			acl_var_udp_queue_dir, acl_last_serror());
	}
#endif

#ifdef ACL_UNIX
	if (user_name) {
		acl_chroot_uid(root_dir, user_name);
	}
#endif

	udp_server_open_log();
	log_event_mode(__event_mode);

	/* 进程初始化完毕后回调此函数，以使用户可以初始化自己的环境 */
	if (post_init) {
		post_init(__service_ctx);
	}

#ifdef ACL_LINUX
	/* notify master that child started ok */
	if (__daemon_mode) {
		acl_master_notify(acl_var_udp_pid, __udp_server_generation,
			ACL_MASTER_STAT_START_OK);
	}
#endif

	/* 设置 SIGHUP 信号 */
	acl_server_sighup_setup();
	acl_server_sigterm_setup();

	acl_msg_info("%s -- %s: daemon started", argv[0], myname);

	servers_start(__servers, acl_var_udp_threads);
}

#endif /* ACL_CLIENT_ONLY */

