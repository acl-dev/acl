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

int   acl_var_udp_threads_detached;
int   acl_var_udp_non_block;
int   acl_var_udp_reuse_port;

static ACL_CONFIG_BOOL_TABLE __conf_bool_tab[] = {
	{ ACL_VAR_UDP_THREADS_DETACHED, ACL_DEF_UDP_THREADS_DETACHED,
		&acl_var_udp_threads_detached },
	{ ACL_VAR_UDP_NON_BLOCK, ACL_DEF_UDP_NON_BLOCK,
		&acl_var_udp_non_block },
	{ ACL_VAR_UDP_REUSEPORT, ACL_DEF_UDP_REUSEPORT,
		&acl_var_udp_reuse_port},

	{ 0, 0, 0 },
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

typedef struct UDP_SERVER {
	acl_pthread_t tid;
	ACL_EVENT    *event;
	ACL_VSTREAM **streams;
	int           count;
	int           size;

	ACL_VSTREAM  *ipc_in;
	ACL_VSTREAM  *ipc_out;
	acl_pthread_mutex_t *lock;
} UDP_SERVER;

typedef struct UDP_CTX {
	ACL_VSTREAM **streams;
	int           count;
	ACL_ARGV     *addrs;
} UDP_CTX;

 /*
  * Global state.
  */
static ACL_UDP_SERVER_FN                __service_main;
static ACL_MASTER_SERVER_EXIT_FN        __service_exit;
static ACL_MASTER_SERVER_THREAD_INIT_FN __thread_init;
static ACL_MASTER_SERVER_SIGHUP_FN      __sighup_handler;
static ACL_MASTER_SERVER_ON_BIND_FN	__server_on_bind;

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

#define SOCK	ACL_VSTREAM_SOCK

/* forward functions */

static ACL_VSTREAM *server_bind_one(const char *addr);
static void udp_server_read(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context);

#ifdef ACL_LINUX
# ifdef SO_REUSEPORT

#include <linux/netlink.h>
#include <linux/route.h>
#include <linux/rtnetlink.h>

#define	IPC_TYPE_ADD	1
#define	IPC_TYPE_DEL	2

typedef struct IPC_DAT {
	int      type;
	UDP_CTX *ctx;
} IPC_DAT;

static ACL_VSTREAM *__if_monitor;

static UDP_CTX *new_ctx(int n)
{
	UDP_CTX *ctx = (UDP_CTX *) acl_mycalloc(1, sizeof(UDP_CTX));
	ctx->count   = 0;
	ctx->streams = (ACL_VSTREAM **) acl_mycalloc(n, sizeof(ACL_VSTREAM*));

	return ctx;
}

static void free_ctx(UDP_CTX *ctx)
{
	int  i;
	for (i = 0; i < ctx->count; i++) {
		if (ctx->streams[i])
			acl_vstream_close(ctx->streams[i]);
	}

	if (ctx->addrs)
		acl_argv_free(ctx->addrs);

	acl_myfree(ctx->streams);
	acl_myfree(ctx);
}

static void server_lock(UDP_SERVER *server)
{
	acl_pthread_mutex_lock(server->lock);
}

static void server_unlock(UDP_SERVER *server)
{
	acl_pthread_mutex_unlock(server->lock);
}

static int stream_addr_equal(ACL_VSTREAM *from, ACL_VSTREAM *to)
{
	char addr1[128], addr2[128];

	if (acl_getsockname(SOCK(from), addr1, sizeof(addr1)) == -1)
		return 0;
	if (acl_getsockname(SOCK(to), addr2, sizeof(addr2)) == -1)
		return 0;
	return strcmp(addr1, addr2) == 0 ? 1 : 0;
}

static int stream_exist(UDP_SERVER *server, ACL_VSTREAM *stream)
{
	int i;

	for (i = 0; i < server->count; i++) {
		if (stream_addr_equal(server->streams[i], stream))
			return 1;
	}

	return 0;
}

static void server_add(UDP_SERVER *server, UDP_CTX *ctx)
{
	int i;

	server_lock(server);

	if (server->count + ctx->count >= server->size) {
		server->size    = server->count + ctx->count + 1;
		server->streams = (ACL_VSTREAM **) acl_myrealloc(
			server->streams, server->size * sizeof(ACL_VSTREAM *));
	}

	for (i = 0; i < ctx->count; i++) {
		if (stream_exist(server, ctx->streams[i])) {
			acl_vstream_close(ctx->streams[i]);
			ctx->streams[i] = NULL;
			continue;
		}

		if (__server_on_bind)
			__server_on_bind(__service_ctx, ctx->streams[i]);
		server->streams[server->count++] = ctx->streams[i];
		acl_event_enable_read(server->event, ctx->streams[i],
			0, udp_server_read, server);
		acl_msg_info("bind %s addr ok, fd %d",
			ACL_VSTREAM_LOCAL(ctx->streams[i]),
			SOCK(ctx->streams[i]));
		ctx->streams[i] = NULL;
	}

	server_unlock(server);
}

static void remove_matched_stream(UDP_SERVER *server, const char *addr)
{
	int i;

	server_lock(server);

	for (i = 0; i < server->count; i++) {
		char local[256];
		ACL_VSTREAM *stream = server->streams[i];

		if (acl_getsockname(SOCK(stream), local, sizeof(local)) == -1)
			continue;

		if (strcmp(local, addr) != 0)
			continue;

		acl_event_disable_readwrite(server->event, stream);
		acl_vstream_close(stream);

		--server->count;
		assert(server->count >= 0);

		if (server->count == 0)
			server->streams[0] = NULL;
		else
			server->streams[i] = server->streams[server->count];
		acl_msg_info("remove one stream ok, addr=%s", addr);
		break;
	}

	server_unlock(server);
}

static void server_del(UDP_SERVER *server, UDP_CTX *ctx)
{
	ACL_ITER iter;

	acl_foreach(iter, ctx->addrs) {
		const char *addr = (const char *) iter.data;
		remove_matched_stream(server, addr);
	}
}

static void server_ipc_read(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream, void *context)
{
	UDP_SERVER *server = (UDP_SERVER *) context;
	IPC_DAT     dat;
	int         ret;

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s(%d): unknown event_type: %d",
			__FUNCTION__, __LINE__, event_type);

	ret = acl_vstream_readn(stream, &dat, sizeof(dat));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): ipc read error %s",
			__FUNCTION__, __LINE__, acl_last_serror());
		return;
	}

	switch (dat.type) {
	case IPC_TYPE_ADD:
		server_add(server, dat.ctx);
		break;
	case IPC_TYPE_DEL:
		server_del(server, dat.ctx);
		break;
	default:
		acl_msg_error("unknown ipc type=%d", dat.type);
		break;
	}

	free_ctx(dat.ctx);
}

static void server_ipc_add(UDP_SERVER *server, UDP_CTX *ctx)
{
	int     ret;
	IPC_DAT dat;

	dat.type = IPC_TYPE_ADD;
	dat.ctx  = ctx;
	ret = acl_vstream_writen(server->ipc_out, &dat, sizeof(dat));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("ipc send error %s", acl_last_serror());
		free_ctx(ctx);
	}
}

static void server_ipc_del(UDP_SERVER *server, UDP_CTX *ctx)
{
	int     ret;
	IPC_DAT dat;

	dat.type = IPC_TYPE_DEL;
	dat.ctx  = ctx;
	ret = acl_vstream_writen(server->ipc_out, &dat, sizeof(dat));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("ipc send error %s", acl_last_serror());
		free_ctx(ctx);
	}
}

static void server_ipc_setup(UDP_SERVER *server)
{
	int i;

	for (i = 0; i < server->count; i++) {
		ACL_SOCKET fds[2];
		int ret = acl_sane_socketpair(AF_UNIX, SOCK_STREAM, 0, fds);
		if (ret < 0) {
			acl_msg_error("socketpair error %s", acl_last_serror());
			continue;
		}

		server->lock    = (acl_pthread_mutex_t *)
			acl_mycalloc(1, sizeof(acl_pthread_mutex_t));
		acl_pthread_mutex_init(server->lock, NULL);

		server->ipc_in  = acl_vstream_fdopen(fds[0], O_RDONLY,
			4096, 0, ACL_VSTREAM_TYPE_SOCK);
		server->ipc_out = acl_vstream_fdopen(fds[1], O_WRONLY,
			4096, 0, ACL_VSTREAM_TYPE_SOCK);
		acl_event_enable_read(server->event, server->ipc_in,
			0, server_ipc_read, server);
	}
}

static void servers_ipc_setup(UDP_SERVER *servers, int count)
{
	int i;

	for (i = 0; i < count; i++)
		server_ipc_setup(&servers[i]);
}

static void server_add_addrs(UDP_SERVER *server, ACL_HTABLE *addrs)
{
	UDP_CTX  *ctx;
	ACL_ITER  iter;
	int  size = acl_htable_used(addrs);

	if (size <= 0)
		return;

	ctx = new_ctx(size);

	acl_foreach(iter, addrs) {
		ACL_VSTREAM *stream = server_bind_one(iter.key);
		if (stream == NULL)
			continue;

		ctx->streams[ctx->count++] = stream;
	}

	if (ctx->count > 0)
		server_ipc_add(server, ctx);
	else
		free_ctx(ctx);
}

static void server_del_addrs(UDP_SERVER *server, ACL_ARGV *addrs)
{
	UDP_CTX  *ctx;

	if (addrs->argc == 0) {
		acl_argv_free(addrs);
		return;
	}

	ctx = new_ctx(addrs->argc);
	ctx->addrs = addrs;

	server_ipc_del(server, ctx);
}

static ACL_VSTREAM *netlink_open(void)
{
	struct sockaddr_nl sa;
	int fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	ACL_VSTREAM *stream;

	if (fd < 0) {
		acl_msg_error("%s(%d), %s: create raw socket error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return NULL;
	}

	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV4_ROUTE |
		RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE;
	if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
		acl_msg_error("%s(%d), %s: bind raw socket error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		close(fd);
		return NULL;
	}

	stream = acl_vstream_fdopen(fd, O_RDWR, 8192, 0, ACL_VSTREAM_TYPE_SOCK);
	return stream;
}

static void server_rebinding(UDP_SERVER *server, ACL_HTABLE *addrs)
{
	ACL_ARGV *addrs2del = acl_argv_alloc(10);
	int i;

	server_lock(server);

	for (i = 0; i < server->count; i++) {
		char buf[128];
		ACL_VSTREAM *stream = server->streams[i];

		if (acl_getsockname(SOCK(stream), buf, sizeof(buf)) == -1)
			continue;

		if (acl_htable_locate(addrs, buf) == NULL)
			acl_argv_add(addrs2del, buf, NULL);
		else
			acl_htable_delete(addrs, buf, NULL);
	}

	server_unlock(server);

	server_add_addrs(server, addrs);
	server_del_addrs(server, addrs2del);
}

static void servers_rebinding(void)
{
	ACL_ARGV   *addrs = acl_ifconf_search(__service_name);
	int         i;


	for (i = 0; i < __nservers; i++) {
		ACL_HTABLE *table = acl_htable_create(10, 0);
		ACL_ITER    iter;
		acl_foreach(iter, addrs) {
			acl_htable_enter(table, (const char *) iter.data, NULL);
		}
		server_rebinding(&__servers[i], table);
		acl_htable_free(table, NULL);
	}

	acl_argv_free(addrs);
}

static void netlink_handle(struct nlmsghdr *nh, unsigned int dlen)
{
	int  changed = 0;

	for (; NLMSG_OK(nh, dlen); nh = NLMSG_NEXT(nh, dlen)) {
		switch (nh->nlmsg_type) {
		case RTM_NEWADDR:
		case RTM_DELADDR:
		case RTM_NEWROUTE:
		case RTM_DELROUTE:
			changed = 1;
			break;
		default:
			break;
		}
	}

	if (changed)
		servers_rebinding();
}

static void netlink_callback(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream, void *context acl_unused)
{
	int  ret;
	char buf[4096];

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type: %d",
			__FILE__, __FUNCTION__, __LINE__, event_type);

	ret = acl_vstream_read(stream, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s, %s(%d): read error %s",
			__FILE__, __FUNCTION__, __LINE__, acl_last_serror());
	} else if (ret < (int) sizeof(struct nlmsghdr)) {
		acl_msg_error("%s, %s(%d): invalid read length=%d",
			__FILE__, __FUNCTION__, __LINE__, ret);
	} else {
		struct nlmsghdr *nh = (struct nlmsghdr *) buf;
		netlink_handle(nh, (unsigned int) ret);
	}
}

# endif	// SO_REUSEPORT
#endif	// ACL_LINUX

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
		UDP_SERVER *server = (UDP_SERVER *)
			acl_pthread_getspecific(__server_key);
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
	UDP_SERVER *server = (UDP_SERVER *)
		acl_pthread_getspecific(__server_key);
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

	if (__service_exit)
		__service_exit(__service_ctx);

	for (i = 0; __conf_str_tab[i].name != NULL; i++)
		acl_myfree(*__conf_str_tab[i].target);
	acl_app_conf_unload();

	if (acl_var_udp_procname)
		acl_myfree(acl_var_udp_procname);

//	if (0)
//		servers_stop();

	if (__main_event)
		acl_event_free(__main_event);

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

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type: %d",
			__FILE__, myname, __LINE__, event_type);

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

	if (procname == NULL || *procname == 0)
		acl_msg_fatal("%s(%d); procname null", myname, __LINE__);

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
	char     local[64];

	if (acl_var_udp_non_block)
		flag |= ACL_INET_FLAG_NBLOCK;
	if (acl_var_udp_reuse_port)
		flag |= ACL_INET_FLAG_REUSEPORT;

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

static void server_binding(UDP_SERVER *server, ACL_ARGV *addrs)
{
	ACL_ITER iter;
	int i = 0;

	acl_foreach(iter, addrs) {
		ACL_VSTREAM *stream = server_bind_one((char *) iter.data);
		acl_event_enable_read(server->event, stream,
			0, udp_server_read, server);
		server->streams[i++] = stream;
		acl_msg_info("bind %s addr ok, fd %d",
			(char *) iter.data, SOCK(stream));
	}

	if (i == 0)
		acl_msg_fatal("%s(%d), %s: binding all addrs failed!",
			__FILE__, __LINE__, __FUNCTION__);
}

static UDP_SERVER *servers_binding(const char *service,
	int event_mode, int nthreads)
{
	ACL_ARGV *addrs = acl_ifconf_search(service);
	UDP_SERVER *servers;
	int i = 0;

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
# ifndef SO_REUSEPORT
static void server_open(UDP_SERVER *server, int sock_count)
{
	ACL_SOCKET fd = ACL_MASTER_LISTEN_FD;
	int i = 0;

	/* socket count is as same listen_fd_count in parent process */

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

static UDP_SERVER *servers_open(int event_mode, int nthreads, int sock_count)
{
	UDP_SERVER *servers;
	int i;

	servers = servers_alloc(event_mode, nthreads, sock_count);

	for (i = 0; i < nthreads; i++)
		server_open(&servers[i], sock_count);

	return servers;
}
# endif
#endif /* ACL_UNIX */

static UDP_SERVER *servers_create(const char *service, int nthreads)
{
	UDP_SERVER *servers = NULL;

	if (strcasecmp(acl_var_udp_event_mode, "poll") == 0)
		__event_mode = ACL_EVENT_POLL;
	else if (strcasecmp(acl_var_udp_event_mode, "kernel") == 0)
		__event_mode = ACL_EVENT_KERNEL;
	else
		__event_mode = ACL_EVENT_SELECT;

	__main_event = acl_event_new(__event_mode, 0,
		acl_var_udp_delay_sec, acl_var_udp_delay_usec);

#ifdef ACL_UNIX
	if (__daemon_mode) {
# ifdef SO_REUSEPORT
		servers = servers_binding(service, __event_mode, nthreads);
# else
		/* __socket_count from command argv */
		servers = servers_open(__event_mode, nthreads, __socket_count);
# endif
	} else
		servers = servers_binding(service, __event_mode, nthreads);

# ifdef ACL_LINUX
#  ifdef SO_REUSEPORT
	/* create monitor watching the network's changing status */
	__if_monitor = netlink_open();
	if (__if_monitor) {
		acl_event_enable_read(__main_event, __if_monitor, 0,
				netlink_callback, servers);
		servers_ipc_setup(servers, nthreads);
		acl_msg_info("--- monitoring ifaddr status ---");
	}
#  endif
# endif

#else	/* !ACL_UNIX */
	if (__daemon_mode) {
		acl_msg_fatal("%s(%d): not support daemon mode!",
			__FILE__, __LINE__);
	} else
		servers = servers_binding(service, __event_mode, nthreads);
#endif /* ACL_UNIX */

	return servers;
}

static void *thread_main(void *ctx)
{
	/* set thread local storage */
	UDP_SERVER *server = (UDP_SERVER *) ctx;
	acl_pthread_setspecific(__server_key, server);

	if (__thread_init)
		__thread_init(__thread_init_ctx);

	while (1)
		acl_event_loop(server->event);

	/* not reached here */
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

	if (acl_var_udp_idle_limit > 0)
		acl_event_request_timer(__main_event,
			udp_server_timeout,
			__main_event,
			(acl_int64) acl_var_udp_idle_limit * 1000000, 0);

	while (1) {
		acl_event_loop(__main_event);
#ifdef ACL_UNIX
		if (!acl_var_server_gotsighup || !__sighup_handler)
			continue;

		acl_var_server_gotsighup = 0;
		if (__sighup_handler(__service_ctx, buf) < 0)
			acl_master_notify(acl_var_udp_pid,
				__udp_server_generation,
				ACL_MASTER_STAT_SIGHUP_ERR);
		else
			acl_master_notify(acl_var_udp_pid,
				__udp_server_generation,
				ACL_MASTER_STAT_SIGHUP_OK);
#endif
	}
    
	/* not reached here */

	/* acl_vstring_free(buf); */
}

static void servers_start(UDP_SERVER *servers, int nthreads)
{
	acl_pthread_attr_t attr;
	int i;

	if (nthreads <= 0)
		acl_msg_fatal("%s(%d), %s: invalid nthreads %d",
			__FILE__, __LINE__, __FUNCTION__, nthreads);

	if (__server_on_bind) {
		for (i = 0; i < nthreads; i++) {
			UDP_SERVER *server = &servers[i];
			int j;

			for (j = 0; j < server->count; j++)
				__server_on_bind(__service_ctx,
					server->streams[j]);
		}
	}

	__clock = acl_atomic_clock_alloc();

	acl_pthread_attr_init(&attr);
	if (acl_var_udp_threads_detached)
		acl_pthread_attr_setdetachstate(&attr,
			ACL_PTHREAD_CREATE_DETACHED);

	for (i = 0; i < nthreads; i++)
		acl_pthread_create(&servers[i].tid, &attr,
			thread_main, &servers[i]);

	main_thread_loop();
}

static void thread_server_exit(void *ctx acl_unused)
{
	acl_msg_info("--thread-%lu exit now---",
		(unsigned long) acl_pthread_self());
}

static void usage(int argc, char *argv[])
{
	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc %d", __FILE__, __LINE__, argc);

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

	if (__conf_file[0] && acl_msg_verbose)
		acl_msg_info("%s(%d), %s: configure file=%s", 
			__FILE__, __LINE__, myname, __conf_file);

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

#ifdef ACL_UNIX
	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &__udp_server_generation);
	}
#endif

	/*******************************************************************/

	if (acl_var_udp_threads <= 0)
		acl_var_udp_threads = 1;

	__servers = servers_create(service_name, acl_var_udp_threads);
	if (__servers)
		__nservers = acl_var_udp_threads;

	/* 创建用于线程局部变量的键对象 */
	acl_pthread_key_create(&__server_key, thread_server_exit);

	/* 必须先设置主线程的对象，以便于应用能及时使用 */
	server = &__servers[acl_var_udp_threads - 1];
	acl_pthread_setspecific(__server_key, server);

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

#ifdef ACL_LINUX
	/* notify master that child started ok */
	if (__daemon_mode)
		acl_master_notify(acl_var_udp_pid, __udp_server_generation,
			ACL_MASTER_STAT_START_OK);
#endif

	/* 设置 SIGHUP 信号 */
	acl_server_sighup_setup();
	acl_server_sigterm_setup();

	acl_msg_info("%s -- %s: daemon started", argv[0], myname);

	servers_start(__servers, acl_var_udp_threads);
}
