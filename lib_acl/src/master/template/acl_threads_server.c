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
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_stringops.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/unix/acl_watchdog.h"
#include "net/acl_access.h"
#include "net/acl_listen.h"
#include "net/acl_tcp_ctl.h"
#include "net/acl_sane_socket.h"
#include "event/acl_events.h"

/* Global library. */

#include "../master_flow.h"
#include "../master_params.h"
#include "../master_proto.h"

/* Application-specific */
#include "master/acl_threads_params.h"
#include "master/acl_server_api.h"
#include "master/acl_app_main.h"
#include "template.h"

int   acl_var_threads_pid;
char *acl_var_threads_procname;
char *acl_var_threads_log_file;

int   acl_var_threads_buf_size;
int   acl_var_threads_rw_timeout;
int   acl_var_threads_pool_limit;
int   acl_var_threads_thread_stacksize;
int   acl_var_threads_thread_idle;
int   acl_var_threads_idle_limit;
int   acl_var_threads_use_limit;
int   acl_var_threads_delay_sec;
int   acl_var_threads_delay_usec;
int   acl_var_threads_daemon_timeout;
int   acl_var_threads_master_maxproc;
int   acl_var_threads_max_accept;
int   acl_var_threads_enable_dog;
int   acl_var_threads_quick_abort;
int   acl_var_threads_enable_core;
int   acl_var_threads_max_debug;
int   acl_var_threads_status_notify;
int   acl_var_threads_batadd;
int   acl_var_threads_check_inter;
int   acl_var_threads_qlen_warn;
int   acl_var_threads_schedule_warn;
int   acl_var_threads_schedule_wait;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ ACL_VAR_THREADS_BUF_SIZE, ACL_DEF_THREADS_BUF_SIZE, &acl_var_threads_buf_size, 0, 0 },
	{ ACL_VAR_THREADS_RW_TIMEOUT, ACL_DEF_THREADS_RW_TIMEOUT, &acl_var_threads_rw_timeout, 0, 0 },
	{ ACL_VAR_THREADS_POOL_LIMIT, ACL_DEF_THREADS_POOL_LIMIT, &acl_var_threads_pool_limit, 0, 0 },
	{ ACL_VAR_THREADS_THREAD_STACKSIZE, ACL_DEF_THREADS_THREAD_STACKSIZE, &acl_var_threads_thread_stacksize, 0, 0 },
	{ ACL_VAR_THREADS_THREAD_IDLE, ACL_DEF_THREADS_THREAD_IDLE, &acl_var_threads_thread_idle, 0, 0 },
	{ ACL_VAR_THREADS_IDLE_LIMIT, ACL_DEF_THREADS_IDLE_LIMIT, &acl_var_threads_idle_limit, 0, 0 },
	{ ACL_VAR_THREADS_DELAY_SEC, ACL_DEF_THREADS_DELAY_SEC, &acl_var_threads_delay_sec, 0, 0 },
	{ ACL_VAR_THREADS_DELAY_USEC, ACL_DEF_THREADS_DELAY_USEC, &acl_var_threads_delay_usec, 0, 0 },
	{ ACL_VAR_THREADS_DAEMON_TIMEOUT, ACL_DEF_THREADS_DAEMON_TIMEOUT, &acl_var_threads_daemon_timeout, 0, 0 },
	{ ACL_VAR_THREADS_USE_LIMIT, ACL_DEF_THREADS_USE_LIMIT, &acl_var_threads_use_limit, 0, 0 },
	{ ACL_VAR_THREADS_MASTER_MAXPROC, ACL_DEF_THREADS_MASTER_MAXPROC, &acl_var_threads_master_maxproc, 0, 0},
	{ ACL_VAR_THREADS_MAX_ACCEPT, ACL_DEF_THREADS_MAX_ACCEPT, &acl_var_threads_max_accept, 0, 0 },
	{ ACL_VAR_THREADS_ENABLE_DOG, ACL_DEF_THREADS_ENABLE_DOG, &acl_var_threads_enable_dog, 0, 0 },
	{ ACL_VAR_THREADS_QUICK_ABORT, ACL_DEF_THREADS_QUICK_ABORT, &acl_var_threads_quick_abort, 0, 0 },
	{ ACL_VAR_THREADS_ENABLE_CORE, ACL_DEF_THREADS_ENABLE_CORE, &acl_var_threads_enable_core, 0, 0 },
	{ ACL_VAR_THREADS_MAX_DEBUG, ACL_DEF_THREADS_MAX_DEBUG, &acl_var_threads_max_debug, 0, 0 },
	{ ACL_VAR_THREADS_STATUS_NOTIFY, ACL_DEF_THREADS_STATUS_NOTIFY, &acl_var_threads_status_notify, 0, 0 },
	{ ACL_VAR_THREADS_BATADD, ACL_DEF_THREADS_BATADD, &acl_var_threads_batadd, 0, 0 },
	{ ACL_VAR_THREADS_QLEN_WARN, ACL_DEF_THREADS_QLEN_WARN, &acl_var_threads_qlen_warn, 0, 0 },
	{ ACL_VAR_THREADS_SCHEDULE_WARN, ACL_DEF_THREADS_SCHEDULE_WARN, &acl_var_threads_schedule_warn, 0, 0 },
	{ ACL_VAR_THREADS_SCHEDULE_WAIT, ACL_DEF_THREADS_SCHEDULE_WAIT, &acl_var_threads_schedule_wait, 0, 0 },
	{ ACL_VAR_THREADS_CHECK_INTER, ACL_DEF_THREADS_CHECK_INTER, &acl_var_threads_check_inter, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

char *acl_var_threads_queue_dir;
char *acl_var_threads_owner;
char *acl_var_threads_event_mode;
char *acl_var_threads_log_debug;
char *acl_var_threads_deny_banner;
char *acl_var_threads_access_allow;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ ACL_VAR_THREADS_QUEUE_DIR, ACL_DEF_THREADS_QUEUE_DIR, &acl_var_threads_queue_dir },
	{ ACL_VAR_THREADS_OWNER, ACL_DEF_THREADS_OWNER, &acl_var_threads_owner },
	{ ACL_VAR_THREADS_EVENT_MODE, ACL_DEF_THREADS_EVENT_MODE, &acl_var_threads_event_mode },
	{ ACL_VAR_THREADS_LOG_DEBUG, ACL_DEF_THREADS_LOG_DEBUG, &acl_var_threads_log_debug },
	{ ACL_VAR_THREADS_DENY_BANNER, ACL_DEF_THREADS_DENY_BANNER, &acl_var_threads_deny_banner },
	{ ACL_VAR_THREADS_ACCESS_ALLOW, ACL_DEF_THREADS_ACCESS_ALLOW, &acl_var_threads_access_allow },

        { 0, 0, 0 },
};

 /*
  * Global state.
  */
static int __client_count;
static int __use_count;
static int __use_limit_delay = 1;
static int __socket_count = 1;
static int __listen_disabled = 0;

static ACL_EVENT *__event = NULL;
static acl_pthread_pool_t *__threads = NULL;
static ACL_VSTREAM **__sstreams;

static time_t __last_closing_time = 0;
static pthread_mutex_t __closing_time_mutex;
static pthread_mutex_t __counter_mutex;

static int (*__service_main) (ACL_VSTREAM *, void *);
static void *__service_ctx;
static char *__service_name;
static char **__service_argv;
static void (*__server_accept) (int, ACL_EVENT *, ACL_VSTREAM *, void *);
static ACL_APP_EXIT_FN __server_onexit;
static void *__server_onexit_ctx;
static unsigned __server_generation;
static ACL_APP_ON_ACCEPT __server_on_accept;
static ACL_APP_ON_CLOSE __server_on_close;
static ACL_APP_ON_TIMEOUT __server_on_timeout;
static char *__deny_info = NULL;

static void lock_closing_time(void)
{
	pthread_mutex_lock(&__closing_time_mutex);
}

static void unlock_closing_time(void)
{
	pthread_mutex_unlock(&__closing_time_mutex);
}

static void lock_counter(void)
{
	pthread_mutex_lock(&__counter_mutex);
}

static void unlock_counter(void)
{
	pthread_mutex_unlock(&__counter_mutex);
}

static void update_closing_time(void)
{
	lock_closing_time();
	__last_closing_time = time(NULL);
	unlock_closing_time();
}

static time_t last_closing_time(void)
{
	time_t  last;

	lock_closing_time();
	last = __last_closing_time;
	unlock_closing_time();

	return last;
}

static void increase_client_counter(void)
{
	lock_counter();
	__client_count++;
	unlock_counter();
}

static void decrease_client_counter(void)
{
	lock_counter();
	__client_count--;
	unlock_counter();
}

static int get_client_count(void)
{
	int   n;

	lock_counter();
	n = __client_count;
	unlock_counter();

	return n;
}

ACL_EVENT *acl_threads_server_event(void)
{
	return __event;
}

acl_pthread_pool_t *acl_threads_server_threads(void)
{
	return __threads;
}

ACL_VSTREAM **acl_threads_server_streams(void)
{
	if (__sstreams == NULL)
		acl_msg_warn("server streams NULL!");
	return __sstreams;
}

static void close_listen_timer(int type acl_unused, ACL_EVENT *event,
	void *context acl_unused)
{
	int   i;

	if (__sstreams == NULL)
		return;

	for (i = 0; __sstreams[i] != NULL; i++) {
		acl_event_disable_readwrite(event, __sstreams[i]);
		acl_vstream_close(__sstreams[i]);
		__sstreams[i] = NULL;
		acl_msg_info("All listener closed now!");
	}
	acl_myfree(__sstreams);
	__sstreams = NULL;
}

static void disable_listen(void)
{
	if (__sstreams == NULL)
		return;

	/**
	 * 只所以采用定时器关闭监听流，一方面因为监听流在事件集合中是“常驻留”的，
	 * 另一方面本线程与事件循环主线程是不同的线程空间，如果在本线程直接关闭
	 * 监听流，会造成事件循环主线程在 select() 时报描述符非法，而当加了定时器
	 * 关闭方法后，定时器的运行线程空间与事件循环的运行线程空间是相同的，所以
	 * 不会造成冲突。这主要因为事件循环线程中先执行 select(), 后执行定时器，如果
	 * select() 执行后定时器启动并将监听流从事件集合中删除，则即使该监听流已经
	 * 准备好也会因其从事件集合中被删除而不会被触发，这样在下次事件循环时 select()
	 * 所调用的事件集合中就不存在该监听流了。
	 */
	acl_event_request_timer(__event, close_listen_timer, NULL, 1000000, 0);
}

/* server_exit - normal termination */

static void server_exit(void)
{
	if (__server_onexit)
		__server_onexit(__server_onexit_ctx);

	exit(0);
}

/* server_timeout - idle time exceeded */

static void server_timeout(int type acl_unused, ACL_EVENT *event, void *context)
{
	const char* myname = "server_timeout";
	time_t last, inter;
	int   n;

	n = get_client_count();

	/* if some fds not be closed, the timer should be reset again */
	if (n > 0 && acl_var_threads_idle_limit > 0) {
		acl_event_request_timer(event, server_timeout, context,
			(acl_int64) acl_var_threads_idle_limit * 1000000, 0);
		return;
	}

	last  = last_closing_time();
	inter = time(NULL) - last;

	if (inter >= 0 && inter < acl_var_threads_idle_limit) {
		acl_event_request_timer(event, server_timeout, context,
			(acl_int64) (acl_var_threads_idle_limit - inter)
				* 1000000, 0);
		return;
	}

	if (acl_msg_verbose)
		acl_msg_info("%s: idle timeout -- exiting", myname);

	server_exit();
}

/* server_abort - terminate after abnormal master exit */

static void server_abort(int event_type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	static int  __aborting = 0;
	const char *myname = "server_abort";
	int   n;

	if (__aborting)
		return;

	if (acl_var_threads_quick_abort) {
		acl_msg_info("%s: master disconnect -- quick exiting", myname);
		server_exit();
	}

	if (!__listen_disabled) {
		__listen_disabled = 1;
		disable_listen();
	}
	
	__aborting = 1;

	n = get_client_count();
	if (n > 0) {
		acl_msg_info("%s: waiting for connections(%d)", myname, n);
		/* set idle timeout to 1 second, one second check once */
		acl_var_threads_idle_limit = 1;
		acl_event_request_timer(event, server_timeout, context,
			(acl_int64) acl_var_threads_idle_limit * 1000000, 0);
		return;
	}

	acl_msg_info("%s: master disconnect -- exiting", myname);
	server_exit();
}

static void server_use_timer(int type acl_unused,
	ACL_EVENT *event, void *context)
{
	int   n;

	n = get_client_count();

	if (n > 0 || __use_count < acl_var_threads_use_limit) {
		acl_event_request_timer(event, server_use_timer, context,
			(acl_int64) __use_limit_delay * 1000000, 0);
		return;
	}

	if (acl_msg_verbose)
		acl_msg_info("use limit -- exiting");

	server_exit();
}

typedef struct {
	acl_pthread_pool_t *threads;
	acl_pthread_job_t *job;
	ACL_VSTREAM *stream;
	ACL_EVENT *event;
	int   event_type;
	void  (*read_callback)(int, ACL_EVENT*, ACL_VSTREAM*, void*);
	int   (*serv_callback)(ACL_VSTREAM*, void*);
	void *serv_arg;
} READ_CTX;

static void thread_callback(void *arg)
{
	READ_CTX *ctx = (READ_CTX*) arg;
	int   ret;

	switch (ctx->event_type) {
	case ACL_EVENT_READ:
		ret = ctx->serv_callback(ctx->stream, ctx->serv_arg);
		if (ret < 0) {
			if (__server_on_close != NULL)
				__server_on_close(ctx->stream, ctx->serv_arg);
			acl_vstream_close(ctx->stream);
		} else if (ret == 0)
			acl_event_enable_read(ctx->event, ctx->stream,
				ctx->stream->rw_timeout,
				ctx->read_callback, ctx);
		break;
	case ACL_EVENT_RW_TIMEOUT:
		if (__server_on_timeout == NULL) {
			if (__server_on_close != NULL)
				__server_on_close(ctx->stream, ctx->serv_arg);
			acl_vstream_close(ctx->stream);
		} else if (__server_on_timeout(ctx->stream, ctx->serv_arg) < 0) {
			if (__server_on_close != NULL)
				__server_on_close(ctx->stream, ctx->serv_arg);
			acl_vstream_close(ctx->stream);
		} else
			acl_event_enable_read(ctx->event, ctx->stream,
				ctx->stream->rw_timeout,
				ctx->read_callback, ctx);
		break;
	case ACL_EVENT_XCPT:
		if (__server_on_close != NULL)
			__server_on_close(ctx->stream, ctx->serv_arg);
		acl_vstream_close(ctx->stream);
		break;
	default:
		acl_msg_fatal("%s, %s(%d): unknown event type(%d)",
			__FILE__, __FUNCTION__, __LINE__, ctx->event_type);
		break;
	}
}

static void read_callback1(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	READ_CTX *ctx = (READ_CTX*) context;
	ctx->event_type = event_type;
	acl_pthread_pool_bat_add_job(ctx->threads, ctx->job);
}

static void read_callback2(int event_type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	READ_CTX *ctx = (READ_CTX*) context;
	ctx->event_type = event_type;
	acl_pthread_pool_add_job(ctx->threads, ctx->job);
}

static void event_fire_begin(ACL_EVENT *event acl_unused, void *ctx)
{
	acl_pthread_pool_t *threads = (acl_pthread_pool_t*) ctx;
	acl_pthread_pool_bat_add_begin(threads);
}

static void event_fire_end(ACL_EVENT *event acl_unused, void *ctx)
{
	acl_pthread_pool_t *threads = (acl_pthread_pool_t*) ctx;
	acl_pthread_pool_bat_add_end(threads);
}

static void free_ctx(ACL_VSTREAM *stream acl_unused, void *context)
{
	READ_CTX *ctx = (READ_CTX*) context;
	if (ctx->job)
		acl_pthread_pool_free_job(ctx->job);
	acl_myfree(ctx);
}

static void server_execute(ACL_EVENT *event, acl_pthread_pool_t *threads,
	ACL_VSTREAM *stream)
{
	READ_CTX *ctx;

	if (acl_var_threads_status_notify && acl_var_threads_master_maxproc > 1
	    && acl_master_notify(acl_var_threads_pid, __server_generation,
		ACL_MASTER_STAT_TAKEN) < 0)
	{
		server_abort(ACL_EVENT_NULL_TYPE, event, stream,
			ACL_EVENT_NULL_CONTEXT);
	}

	if (stream->ioctl_read_ctx == NULL) {
		ctx = (READ_CTX*) acl_mymalloc(sizeof(READ_CTX));
		ctx->stream        = stream;
		ctx->threads       = threads;
		ctx->event         = event;
		ctx->event_type    = -1;
		ctx->serv_callback = __service_main;
		ctx->serv_arg      = __service_ctx;
		ctx->job = acl_pthread_pool_alloc_job(thread_callback, ctx, 1);

		if (acl_var_threads_batadd)
			ctx->read_callback = read_callback1;
		else
			ctx->read_callback = read_callback2;

		stream->ioctl_read_ctx = ctx;
		acl_vstream_add_close_handle(stream, free_ctx, ctx);
	} else
		ctx = (READ_CTX*) stream->ioctl_read_ctx;

	acl_event_enable_read(event, stream, stream->rw_timeout,
		ctx->read_callback, ctx);

	if (acl_var_threads_status_notify && acl_var_threads_master_maxproc > 1
	    && acl_master_notify(acl_var_threads_pid, __server_generation,
		ACL_MASTER_STAT_AVAIL) < 0)
	{
		server_abort(ACL_EVENT_NULL_TYPE, event, stream,
			ACL_EVENT_NULL_CONTEXT);
	}
}

static void decrease_counter_callback(ACL_VSTREAM *stream acl_unused,
	void *arg acl_unused)
{
	update_closing_time();
	decrease_client_counter();
}

static void server_wakeup(ACL_EVENT *event, acl_pthread_pool_t *threads,
	int fd, const char *remote, const char *local)
{
	ACL_VSTREAM *stream;
	char  addr[64];

	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
	increase_client_counter();

	__use_count++;

	stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_threads_buf_size,
			acl_var_threads_rw_timeout, ACL_VSTREAM_TYPE_SOCK);

	if (remote) {
		char *ptr;
		acl_vstream_set_peer(stream, remote);
		ACL_SAFE_STRNCPY(addr, remote, sizeof(addr));
		ptr = strchr(addr, ':');
		if (ptr)
			*ptr = 0;
	} else
		addr[0] = 0;
	if (local)
		acl_vstream_set_local(stream, local);

	/**
	 * when the stream is closed, the callback will be called
	 * to decrease the counter
	 */
	acl_vstream_add_close_handle(stream, decrease_counter_callback, NULL);

	if (__server_on_accept) {
		if (__server_on_accept(stream) < 0) {
			acl_vstream_close(stream);
			return;
		}
	}

	if (addr[0] != 0 && !acl_access_permit(addr)) {
		if (__deny_info && *__deny_info)
			(void) acl_vstream_fprintf(stream, "%s\r\n", __deny_info);
		acl_vstream_close(stream);
		return;
	}

	server_execute(event, threads, stream);
}

/* restart listening */

static void restart_listen_timer(int type acl_unused,
	ACL_EVENT *event, void *context)
{
	ACL_VSTREAM *stream = (ACL_VSTREAM*) context;

	acl_msg_info("restart listen now!");

	acl_assert(__threads);
	acl_event_enable_listen(event, stream, 0, __server_accept, __threads);
}

/* server_accept_sock - accept client connection request */

static void server_accept_sock(int event_type, ACL_EVENT *event,
	ACL_VSTREAM *stream, void *context)
{
	const char *myname = "server_accept_sock";
	int   listen_fd = ACL_VSTREAM_SOCK(stream);
	int   time_left = -1, i = 0, delay_listen = 0, fd, sock_type;
	char  remote[64], local[64];
	acl_pthread_pool_t *threads = (acl_pthread_pool_t*) context;

	if (__sstreams == NULL) {
		acl_msg_info("Server stoping ...");
		return;
	}

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type(%d)",
			__FILE__, myname, __LINE__, event_type);

	if (acl_var_threads_idle_limit > 0)
		time_left = (int) ((acl_event_cancel_timer(event,
			server_timeout, threads) + 999999) / 1000000);
	else
		time_left = acl_var_threads_idle_limit;

	while (i++ < acl_var_threads_max_accept) {
		fd = acl_accept(listen_fd, remote, sizeof(remote), &sock_type);
		if (fd >= 0) {
			/* set NODELAY for TCP socket */
			if (sock_type == AF_INET)
				acl_tcp_set_nodelay(fd);
			if (acl_getsockname(fd, local, sizeof(local)) < 0)
				memset(local, 0, sizeof(local));
			server_wakeup(event, threads, fd, remote, local);
			continue;
		}

		/* else: fd < 0 */

		if (errno == EMFILE) {
			delay_listen = 1;
			acl_msg_warn("accept connection: %s",
				acl_last_serror());
			break;
		}

		if (errno == EAGAIN || errno == EINTR)
			break;

		acl_msg_warn("accept connection: %s, stoping ...",
			acl_last_serror());
		acl_event_disable_readwrite(event, stream);
		server_abort(0, event, stream, threads);
		return;
	}

	if (delay_listen) {
		acl_event_disable_readwrite(event, stream);
		acl_event_request_timer(event, restart_listen_timer,
			stream, 2000000, 0);
	} else
		acl_event_enable_listen(event, stream, 0,
			__server_accept, threads);

	if (time_left > 0)
		acl_event_request_timer(event, server_timeout, threads,
			(acl_int64) time_left * 1000000, 0);
}

static void server_init(const char *procname)
{
	const char *myname = "server_init";
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
	acl_var_threads_pid = getpid();
	acl_var_threads_procname = acl_mystrdup(acl_safe_basename(procname));

	acl_var_threads_log_file = getenv("SERVICE_LOG");
	if (acl_var_threads_log_file == NULL) {
		acl_var_threads_log_file = acl_mystrdup("acl_master.log");
		acl_msg_warn("%s(%d)->%s: can't get SERVICE_LOG's env value,"
			" use %s log", __FILE__, __LINE__, myname,
			acl_var_threads_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_str_table(__conf_str_tab);

	acl_master_vars_init(acl_var_threads_buf_size, acl_var_threads_rw_timeout);

	if (__deny_info == NULL)
		__deny_info = acl_var_threads_deny_banner;
	if (acl_var_threads_access_allow && *acl_var_threads_access_allow)
		acl_access_add(acl_var_threads_access_allow, ", \t", ":");
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

static void open_log(int event_mode)
{
	/* first, close the master's log */
	master_log_close();

	/* second, open the service's log */
	acl_msg_open(acl_var_threads_log_file, acl_var_threads_procname);

	if (acl_var_threads_log_debug && *acl_var_threads_log_debug
		&& acl_var_threads_max_debug >= 100)
	{
		acl_debug_init2(acl_var_threads_log_debug,
			acl_var_threads_max_debug);
	}
	log_event_mode(event_mode);
}

static ACL_EVENT *event_open(int event_mode, acl_pthread_pool_t *threads)
{
	ACL_EVENT *event;
	
	event = acl_event_new(event_mode, 1, acl_var_threads_delay_sec,
			acl_var_threads_delay_usec);

	/* set the event fire begin and fire end callback */
	if (acl_var_threads_batadd)
		acl_event_set_fire_hook(event, event_fire_begin,
			event_fire_end, threads);

	if (acl_var_threads_check_inter >= 0)
		acl_event_set_check_inter(event, acl_var_threads_check_inter);

	if (acl_var_threads_qlen_warn > 0)
		acl_pthread_pool_set_qlen_warn(threads,
			acl_var_threads_qlen_warn);

	/*
	 * Running as a semi-resident server. Service connection requests.
	 * Terminate when we have serviced a sufficient number of clients,
	 * when no-one has been talking to us for a configurable amount of
	 * time, or when the master process terminated abnormally.
	 */

	if (acl_var_threads_idle_limit > 0)
		acl_event_request_timer(event, server_timeout, event,
			(acl_int64) acl_var_threads_idle_limit * 1000000, 0);

	if (acl_var_threads_use_limit > 0)
		acl_event_request_timer(event, server_use_timer, event,
			(acl_int64) __use_limit_delay * 1000000, 0);

	if (acl_var_threads_enable_dog)
		acl_event_add_dog(event);

	return event;
}

static acl_pthread_pool_t *threads_create(ACL_APP_THREAD_ON_INIT init_fn,
	ACL_APP_THREAD_ON_EXIT exit_fn, void *init_ctx, void *exit_ctx)
{
	acl_pthread_pool_t *threads;

	threads = acl_thread_pool_create(acl_var_threads_pool_limit,
			acl_var_threads_thread_idle);

	if (acl_var_threads_schedule_warn > 0)
		acl_pthread_pool_set_schedule_warn(threads,
			acl_var_threads_schedule_warn);
	if (acl_var_threads_schedule_wait > 0)
		acl_pthread_pool_set_schedule_wait(threads,
			acl_var_threads_schedule_wait);

	if (acl_var_threads_thread_stacksize > 0)
		acl_pthread_pool_set_stacksize(threads,
			acl_var_threads_thread_stacksize);
	if (init_fn)
		acl_pthread_pool_atinit(threads, init_fn, init_ctx);
	if (exit_fn)
		acl_pthread_pool_atfree(threads, exit_fn, exit_ctx);

	pthread_mutex_init(&__closing_time_mutex, NULL);
	pthread_mutex_init(&__counter_mutex, NULL);
	__last_closing_time = time(NULL);

	__use_limit_delay = acl_var_threads_delay_sec > 1 ?
		acl_var_threads_delay_sec : 1;

	return threads;
}

static ACL_VSTREAM **server_open(ACL_EVENT *event, acl_pthread_pool_t *threads,
	int count, int fdtype)
{
	const char *myname = "server_open";
	ACL_VSTREAM *stream, **streams;
	int   fd, i;

	/* socket count is as same listen_fd_count in parent process */

	streams = (ACL_VSTREAM **) acl_mycalloc(count + 1, sizeof(ACL_VSTREAM *));
	for (i = 0; i < count + 1; i++)
		streams[i] = NULL;

	i = 0;
	fd = ACL_MASTER_LISTEN_FD;
	for (; fd < ACL_MASTER_LISTEN_FD + count; fd++) {
		stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_threads_buf_size,
			acl_var_threads_rw_timeout, fdtype);
		if (stream == NULL)
			acl_msg_fatal("%s(%d)->%s: stream null, fd = %d",
				__FILE__, __LINE__, myname, fd);

		acl_non_blocking(fd, ACL_NON_BLOCKING);
		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		acl_event_enable_listen(event, stream, 0, __server_accept, threads);
		streams[i++] = stream;
	}

	acl_event_enable_read(event, ACL_MASTER_STAT_STREAM, 0,
		server_abort, threads);

	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	return streams;
}

static void usage(int argc, char *argv[])
{
	int   i;
	char *service_name;

	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc(%d) invalid",
			__FILE__, __LINE__, argc);

	service_name = acl_mystrdup(acl_safe_basename(argv[0]));

	for (i = 0; i < argc; i++)
		acl_msg_info("argv[%d]: %s", i, argv[i]);

	acl_msg_info("usage: %s -h[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -v [on acl_msg_verbose]"
		" -f conf_file",
		service_name);
}

/* acl_threads_server_main - the real main program */

void acl_threads_server_main(int argc, char **argv,
	int (*service)(ACL_VSTREAM*, void*), void *service_ctx, int name, ...)
{
	const char *myname = "acl_threads_server_main";
	char *root_dir = 0, *user_name = 0, *transport = 0;
	char *service_name = acl_mystrdup(acl_safe_basename(argv[0]));
	int   c, fdtype = 0, event_mode;
	char *generation, conf_file[1024];
	void *thread_init_ctx = NULL, *thread_exit_ctx = NULL;
	ACL_APP_PRE_JAIL pre_jail = 0;
	ACL_APP_INIT_FN  post_init = 0;
	void *pre_jail_ctx = NULL, *post_init_ctx = NULL;
	ACL_APP_THREAD_ON_INIT thread_init_fn = NULL;
	ACL_APP_THREAD_ON_EXIT thread_exit_fn = NULL;
	va_list ap;

	/*******************************************************************/

	/* If not connected to stdin, stdin must not be a terminal. */
	if (isatty(STDIN_FILENO)) {
		printf("%s(%d), %s: do not run this command by hand\r\n",
			__FILE__, __LINE__, myname);
		exit (1);
	}

	/*******************************************************************/

	/* use master's log before init, so can log something */
	acl_master_log_open(argv[0]);

	/*
	 * Pick up policy settings from master process. Shut up error
	 * messages to stderr, because no-one is going to see them.
	 */

	opterr = 0;
	conf_file[0] = 0;

	while ((c = getopt(argc, argv, "hc:n:s:t:uvf:")) > 0) {
		switch (c) {
		case 'h':
			usage(argc, argv);
			exit (0);
		case 'f':
			acl_app_conf_load(optarg);
			ACL_SAFE_STRNCPY(conf_file, optarg, sizeof(conf_file));
			break;
		case 'c':
			root_dir = "setme";
			break;
		case 'n':
			service_name = optarg;
			break;
		case 's':
			if ((__socket_count = atoi(optarg)) <= 0)
				acl_msg_fatal("invalid socket_count: %s", optarg);
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
		default:
			break;
		}
	}

	if (conf_file[0] == 0)
		acl_msg_fatal("%s(%d)->%s: need \"-f pathname\"",
			__FILE__, __LINE__, myname);
	else if (acl_msg_verbose)
		acl_msg_info("%s(%d)->%s: configure file = %s", 
			__FILE__, __LINE__, myname, conf_file);

	/*******************************************************************/

	/* Application-specific initialization. */

	/* load configure, set signal */
	server_init(argv[0]);

	va_start(ap, name);
	for (; name != ACL_APP_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_APP_CTL_CFG_BOOL:
			acl_get_app_conf_bool_table(va_arg(ap, ACL_CONFIG_BOOL_TABLE *));
			break;
		case ACL_APP_CTL_CFG_INT:
			acl_get_app_conf_int_table(va_arg(ap, ACL_CONFIG_INT_TABLE *));
			break;
		case ACL_APP_CTL_CFG_INT64:
			acl_get_app_conf_int64_table(va_arg(ap, ACL_CONFIG_INT64_TABLE *));
			break;
		case ACL_APP_CTL_CFG_STR:
			acl_get_app_conf_str_table(va_arg(ap, ACL_CONFIG_STR_TABLE *));
			break;

		case ACL_APP_CTL_PRE_JAIL:
			pre_jail = va_arg(ap, ACL_APP_PRE_JAIL);
			break;
		case ACL_APP_CTL_PRE_JAIL_CTX:
			pre_jail_ctx = va_arg(ap, void*);
			break;
		case ACL_APP_CTL_INIT_FN:
			post_init = va_arg(ap, ACL_APP_INIT_FN);
			break;
		case ACL_APP_CTL_INIT_CTX:
			post_init_ctx = va_arg(ap, void *);
			break;

		case ACL_APP_CTL_ON_ACCEPT:
			__server_on_accept = va_arg(ap, ACL_APP_ON_ACCEPT);
			break;
		case ACL_APP_CTL_ON_TIMEOUT:
			__server_on_timeout = va_arg(ap, ACL_APP_ON_TIMEOUT);
			break;
		case ACL_APP_CTL_ON_CLOSE:
			__server_on_close = va_arg(ap, ACL_APP_ON_CLOSE);
			break;

		case ACL_APP_CTL_EXIT_FN:
			__server_onexit = va_arg(ap, ACL_APP_EXIT_FN);
			break;
		case ACL_APP_CTL_EXIT_CTX:
			__server_onexit_ctx = va_arg(ap, void *);
			break;

		case ACL_APP_CTL_THREAD_INIT:
			thread_init_fn = va_arg(ap, ACL_APP_THREAD_ON_INIT);
			break;
		case ACL_APP_CTL_THREAD_INIT_CTX:
			thread_init_ctx = va_arg(ap, void*);
			break;
		case ACL_APP_CTL_THREAD_EXIT:
			thread_exit_fn = va_arg(ap, ACL_APP_THREAD_ON_EXIT);
			break;
		case ACL_APP_CTL_THREAD_EXIT_CTX:
			thread_exit_ctx = va_arg(ap, void*);
			break;

		case ACL_APP_CTL_DENY_INFO:
			__deny_info = acl_mystrdup(va_arg(ap, const char*));
			break;
		default:
			acl_msg_fatal("%s: bad name(%d)", myname, name);
		}
	}

	va_end(ap);

	/*******************************************************************/

	if (root_dir)
		root_dir = acl_var_threads_queue_dir;
	if (user_name)
		user_name = acl_var_threads_owner;

	if (transport == NULL)
		acl_msg_fatal("no transport type specified");

	if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_INET) == 0) {
		__server_accept = server_accept_sock;
		fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_UNIX) == 0) {
		__server_accept = server_accept_sock;
		fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_UNIX;
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_SOCK) == 0) {
		__server_accept = server_accept_sock;
		fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
	} else
		acl_msg_fatal("unsupported transport type: %s", transport);

	if (strcasecmp(acl_var_threads_event_mode, "poll") == 0)
		event_mode = ACL_EVENT_POLL;
	else if (strcasecmp(acl_var_threads_event_mode, "kernel") == 0)
		event_mode = ACL_EVENT_KERNEL;
	else
		event_mode = ACL_EVENT_SELECT;

	/*******************************************************************/

	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &__server_generation);
		if (acl_msg_verbose)
			acl_msg_info("process generation: %s (%o)",
				generation, __server_generation);
	}

	/* Set up call-back info. */
	__service_main = service;
	__service_ctx = service_ctx;
	__service_name = service_name;
	__service_argv = argv + optind;

	/*******************************************************************/

	/* Run pre-jail initialization. */
	if (chdir(acl_var_threads_queue_dir) < 0)
		acl_msg_fatal("chdir(\"%s\"): %s", acl_var_threads_queue_dir,
			acl_last_serror());

	/* open the server's log */
	open_log(event_mode);

	/* create threads pool */
	__threads = threads_create(thread_init_fn, thread_exit_fn,
		thread_init_ctx, thread_exit_ctx);

	/* create event */
	__event = event_open(event_mode, __threads);

	/* open all listen streams */
	__sstreams = server_open(__event, __threads, __socket_count, fdtype);

	if (pre_jail)
		pre_jail(pre_jail_ctx);

	acl_chroot_uid(root_dir, user_name);

	/* if enable dump core when program crashed ? */
	if (acl_var_threads_enable_core)
		set_core_limit();

	/* Run post-jail initialization. */
	if (post_init)
		post_init(post_init_ctx);

	acl_msg_info("%s(%d), %s daemon started, log: %s",
		myname, __LINE__, argv[0], acl_var_threads_log_file);

	/*
	 * Traditionally, BSD select() can't handle threadsple processes
	 * selecting on the same socket, and wakes up every process in
	 * select(). See TCP/IP Illustrated volume 2 page 532. We avoid
	 * select() collisions with an external lock file.
	 */
	while (1)
		acl_event_loop(__event);

	/* not reached here */
	server_exit();
}
#endif /* ACL_UNIX */
