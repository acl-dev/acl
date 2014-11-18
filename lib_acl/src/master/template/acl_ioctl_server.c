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
#include "stdlib/unix/acl_core_limit.h"
#include "stdlib/acl_split_at.h"
#include "net/acl_listen.h"
#include "net/acl_tcp_ctl.h"
#include "net/acl_sane_socket.h"
#include "event/acl_events.h"
#include "ioctl/acl_ioctl.h"

/* Global library. */

#include "../master_flow.h"
#include "../master_params.h"
#include "../master_proto.h"

/* Application-specific */
#include "master/acl_ioctl_params.h"
#include "master/acl_server_api.h"
#include "master_log.h"

int   acl_var_ioctl_pid;
char *acl_var_ioctl_procname;
char *acl_var_ioctl_log_file;

int   acl_var_ioctl_buf_size;
int   acl_var_ioctl_rw_timeout;
int   acl_var_ioctl_in_flow_delay;
int   acl_var_ioctl_max_threads;
int   acl_var_ioctl_stacksize;
int   acl_var_ioctl_thread_idle_limit;
int   acl_var_ioctl_idle_limit;
int   acl_var_ioctl_delay_sec;
int   acl_var_ioctl_delay_usec;
int   acl_var_ioctl_daemon_timeout;
int   acl_var_ioctl_use_limit;
int   acl_var_ioctl_master_maxproc;
int   acl_var_ioctl_max_accept;
int   acl_var_ioctl_enable_dog;
int   acl_var_ioctl_quick_abort;
int   acl_var_ioctl_enable_core;
int   acl_var_ioctl_max_debug;
int   acl_var_ioctl_status_notify;
int   acl_var_ioctl_check_inter;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ ACL_VAR_IOCTL_BUF_SIZE, ACL_DEF_IOCTL_BUF_SIZE, &acl_var_ioctl_buf_size, 0, 0 },
	{ ACL_VAR_IOCTL_RW_TIMEOUT, ACL_DEF_IOCTL_RW_TIMEOUT, &acl_var_ioctl_rw_timeout, 0, 0 },
	{ ACL_VAR_IOCTL_IN_FLOW_DELAY, ACL_DEF_IOCTL_IN_FLOW_DELAY, &acl_var_ioctl_in_flow_delay, 0, 0 },
	{ ACL_VAR_IOCTL_MAX_THREADS, ACL_DEF_IOCTL_MAX_THREADS, &acl_var_ioctl_max_threads, 0, 0 },
	{ ACL_VAR_IOCTL_STACKSIZE, ACL_DEF_IOCTL_STACKSIZE, &acl_var_ioctl_stacksize, 0, 0 },
	{ ACL_VAR_IOCTL_THREAD_IDLE_LIMIT, ACL_DEF_IOCTL_THREAD_IDLE_LIMIT, &acl_var_ioctl_thread_idle_limit, 0, 0 },
	{ ACL_VAR_IOCTL_IDLE_LIMIT, ACL_DEF_IOCTL_IDLE_LIMIT, &acl_var_ioctl_idle_limit, 0, 0 },
	{ ACL_VAR_IOCTL_DELAY_SEC, ACL_DEF_IOCTL_DELAY_SEC, &acl_var_ioctl_delay_sec, 0, 0 },
	{ ACL_VAR_IOCTL_DELAY_USEC, ACL_DEF_IOCTL_DELAY_USEC, &acl_var_ioctl_delay_usec, 0, 0 },
	{ ACL_VAR_IOCTL_DAEMON_TIMEOUT, ACL_DEF_IOCTL_DAEMON_TIMEOUT, &acl_var_ioctl_daemon_timeout, 0, 0 },
	{ ACL_VAR_IOCTL_USE_LIMIT, ACL_DEF_IOCTL_USE_LIMIT, &acl_var_ioctl_use_limit, 0, 0 },
	{ ACL_VAR_IOCTL_MASTER_MAXPROC, ACL_DEF_IOCTL_MASTER_MAXPROC, &acl_var_ioctl_master_maxproc, 0, 0},
	{ ACL_VAR_IOCTL_MAX_ACCEPT, ACL_DEF_IOCTL_MAX_ACCEPT, &acl_var_ioctl_max_accept, 0, 0 },
	{ ACL_VAR_IOCTL_ENABLE_DOG, ACL_DEF_IOCTL_ENABLE_DOG, &acl_var_ioctl_enable_dog, 0, 0 },
	{ ACL_VAR_IOCTL_QUICK_ABORT, ACL_DEF_IOCTL_QUICK_ABORT, &acl_var_ioctl_quick_abort, 0, 0 },
	{ ACL_VAR_IOCTL_ENABLE_CORE, ACL_DEF_IOCTL_ENABLE_CORE, &acl_var_ioctl_enable_core, 0, 0 },
	{ ACL_VAR_IOCTL_MAX_DEBUG, ACL_DEF_IOCTL_MAX_DEBUG, &acl_var_ioctl_max_debug, 0, 0 },
	{ ACL_VAR_IOCTL_STATUS_NOTIFY, ACL_DEF_IOCTL_STATUS_NOTIFY, &acl_var_ioctl_status_notify, 0, 0 },
	{ ACL_VAR_IOCTL_CHECK_INTER, ACL_DEF_IOCTL_CHECK_INTER, &acl_var_ioctl_check_inter, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

char *acl_var_ioctl_queue_dir;
char *acl_var_ioctl_owner;
char *acl_var_ioctl_pid_dir;
char *acl_var_ioctl_access_allow;
char *acl_var_ioctl_event_mode;
char *acl_var_ioctl_log_debug;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ ACL_VAR_IOCTL_QUEUE_DIR, ACL_DEF_IOCTL_QUEUE_DIR, &acl_var_ioctl_queue_dir },
	{ ACL_VAR_IOCTL_OWNER, ACL_DEF_IOCTL_OWNER, &acl_var_ioctl_owner },
	{ ACL_VAR_IOCTL_PID_DIR, ACL_DEF_IOCTL_PID_DIR, &acl_var_ioctl_pid_dir },
	{ ACL_VAR_IOCTL_ACCESS_ALLOW, ACL_DEF_IOCTL_ACCESS_ALLOW, &acl_var_ioctl_access_allow },
	{ ACL_VAR_IOCTL_EVENT_MODE, ACL_DEF_IOCTL_EVENT_MODE, &acl_var_ioctl_event_mode },
	{ ACL_VAR_IOCTL_LOG_DEBUG, ACL_DEF_IOCTL_LOG_DEBUG, &acl_var_ioctl_log_debug },

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

static ACL_IOCTL *__h_ioctl = NULL;
static ACL_VSTREAM **__sstreams;

static time_t __last_closing_time = 0;
static pthread_mutex_t __closing_time_mutex;
static pthread_mutex_t __counter_mutex;

static void (*ioctl_server_service) (ACL_IOCTL *, ACL_VSTREAM *, char *, char **);
static char *ioctl_server_name;
static char **ioctl_server_argv;
static void (*ioctl_server_accept) (int, ACL_IOCTL *, ACL_VSTREAM *, void *);
static ACL_MASTER_SERVER_EXIT_FN ioctl_server_onexit;
static unsigned ioctl_server_generation;

static void ioctl_init(void)
{
	acl_assert(pthread_mutex_init(&__closing_time_mutex, NULL) == 0);
	acl_assert(pthread_mutex_init(&__counter_mutex, NULL) == 0);
	__last_closing_time = time(NULL);

	__use_limit_delay = acl_var_ioctl_delay_sec > 1 ?
				acl_var_ioctl_delay_sec : 1;
}

static void lock_closing_time(void)
{
	acl_assert(pthread_mutex_lock(&__closing_time_mutex) == 0);
}

static void unlock_closing_time(void)
{
	acl_assert(pthread_mutex_unlock(&__closing_time_mutex) == 0);
}

static void lock_counter(void)
{
	acl_assert(pthread_mutex_lock(&__counter_mutex) == 0);
}

static void unlock_counter(void)
{
	acl_assert(pthread_mutex_unlock(&__counter_mutex) == 0);
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

void acl_ioctl_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, int delay)
{
	acl_assert(__h_ioctl);
	acl_ioctl_request_timer(__h_ioctl, timer_fn, arg,
		(acl_int64) delay * 1000000);
}

void acl_ioctl_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn, void *arg)
{
	acl_assert(__h_ioctl);
	acl_ioctl_cancel_timer(__h_ioctl, timer_fn, arg);
}

ACL_EVENT *acl_ioctl_server_event()
{
	return acl_ioctl_event(__h_ioctl);
}

ACL_IOCTL *acl_ioctl_server_handle()
{
	return __h_ioctl;
}

ACL_VSTREAM **acl_ioctl_server_streams()
{
	if (__sstreams == NULL)
		acl_msg_warn("server streams NULL!");
	return __sstreams;
}

static void close_listen_timer(int type acl_unused, ACL_EVENT *event acl_unused,
	void *context acl_unused)
{
	int   i;

	if (__sstreams == NULL)
		return;
	for (i = 0; __sstreams[i] != NULL; i++) {
		acl_ioctl_disable_readwrite(__h_ioctl, __sstreams[i]);
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
	acl_ioctl_request_timer(__h_ioctl, close_listen_timer, NULL, 1000000);
}

/* ioctl_server_exit - normal termination */

static void ioctl_server_exit(void)
{
	if (ioctl_server_onexit)
		ioctl_server_onexit(ioctl_server_name);
	/*
		ioctl_server_onexit(ioctl_server_name, ioctl_server_argv);
		*/

	/* XXX: some bugs exist, need to be fixed --- zsx
	if (__h_ioctl)
		acl_ioctl_free(__h_ioctl);
	*/

	exit(0);
}

/* ioctl_server_timeout - idle time exceeded */

static void ioctl_server_timeout(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	const char* myname = "ioctl_server_timeout";
	ACL_IOCTL *h_ioctl = (ACL_IOCTL *) context;
	time_t last, inter;
	int   n;

	n = get_client_count();

	/* if there are some fds not be closed, the timer should be reset again */
	if (n > 0 && acl_var_ioctl_idle_limit > 0) {
		acl_ioctl_request_timer(h_ioctl, ioctl_server_timeout, h_ioctl,
			(acl_int64) acl_var_ioctl_idle_limit * 1000000);
		return;
	}

	last  = last_closing_time();
	inter = time(NULL) - last;

	if (inter >= 0 && inter < acl_var_ioctl_idle_limit) {
		acl_ioctl_request_timer(h_ioctl, ioctl_server_timeout, h_ioctl,
			(acl_int64) (acl_var_ioctl_idle_limit - inter) * 1000000);
		return;
	}

	if (acl_msg_verbose)
		acl_msg_info("%s: idle timeout -- exiting", myname);

	ioctl_server_exit();
}

/* ioctl_server_abort - terminate after abnormal master exit */

static void ioctl_server_abort(int event acl_unused, ACL_IOCTL *h_ioctl,
	ACL_VSTREAM *stream acl_unused, void *context acl_unused)
{
	static int  __aborting = 0;
	const char *myname = "ioctl_server_abort";
	int   n;

	if (__aborting)
		return;

	if (acl_var_ioctl_quick_abort) {
		acl_msg_info("%s: master disconnect -- quick exiting", myname);
		ioctl_server_exit();
	}

	if (!__listen_disabled) {
		__listen_disabled = 1;
		disable_listen();
	}
	
	__aborting = 1;

	n = get_client_count();
	if (n > 0) {
		acl_msg_info("%s: wait for all connection(count=%d) closing",
			myname, n);
		/* set idle timeout to 1 second, one second check once */
		acl_var_ioctl_idle_limit = 1;
		acl_ioctl_request_timer(h_ioctl, ioctl_server_timeout,
			h_ioctl, (acl_int64) acl_var_ioctl_idle_limit * 1000000);
		return;
	}

	acl_msg_info("%s: master disconnect -- exiting", myname);
	ioctl_server_exit();
}

static void ioctl_server_use_timer(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	ACL_IOCTL *h_ioctl = (ACL_IOCTL *) context;
	int   n;

	n = get_client_count();

	if (n > 0 || __use_count < acl_var_ioctl_use_limit) {
		acl_ioctl_request_timer(h_ioctl, ioctl_server_use_timer,
			h_ioctl, (acl_int64) __use_limit_delay * 1000000);
		return;
	}

	if (acl_msg_verbose)
		acl_msg_info("use limit -- exiting");

	ioctl_server_exit();
}

void acl_ioctl_server_enable_read(ACL_IOCTL *h_ioctl, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN notify_fn, void *context)
{
	const char *myname = "acl_ioctl_server_enable_read";

	if (h_ioctl == NULL || stream == NULL || ACL_VSTREAM_SOCK(stream) < 0) {
		acl_msg_error("%s(%d): input error", myname, __LINE__);
		return;
	}
	/* __client_count++; */
	acl_ioctl_enable_read(h_ioctl, stream, timeout, notify_fn, context);
}

/* ioctl_server_execute - in case (char *) != (struct *) */

static void ioctl_server_execute(ACL_IOCTL *h_ioctl, ACL_VSTREAM *stream)
{
	if (acl_var_ioctl_status_notify && acl_var_ioctl_master_maxproc > 1
	    && acl_master_notify(acl_var_ioctl_pid, ioctl_server_generation,
		ACL_MASTER_STAT_TAKEN) < 0)
	{
		ioctl_server_abort(ACL_EVENT_NULL_TYPE, h_ioctl, stream,
			ACL_EVENT_NULL_CONTEXT);
	}

	ioctl_server_service(h_ioctl, stream, ioctl_server_name, ioctl_server_argv);

	if (acl_var_ioctl_status_notify && acl_var_ioctl_master_maxproc > 1
	    && acl_master_notify(acl_var_ioctl_pid, ioctl_server_generation,
		ACL_MASTER_STAT_AVAIL) < 0)
	{
		ioctl_server_abort(ACL_EVENT_NULL_TYPE, h_ioctl, stream,
			ACL_EVENT_NULL_CONTEXT);
	}
}

static void decrease_counter_callback(ACL_VSTREAM *stream acl_unused,
	void *arg acl_unused)
{
	update_closing_time();
	decrease_client_counter();
}

/* ioctl_server_wakeup - wake up application */

static void ioctl_server_wakeup(ACL_IOCTL *h_ioctl, int fd,
	const char *remote, const char *local)
{
	ACL_VSTREAM *stream;

	if (acl_msg_verbose)
		acl_msg_info("connection established fd %d", fd);

/*
	acl_non_blocking(fd, ACL_BLOCKING);
*/
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	increase_client_counter();

	__use_count++;

	stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_ioctl_buf_size,
			acl_var_ioctl_rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	if (stream == NULL)
		acl_msg_fatal("%s(%d): acl_vstream_fdopen error(%s)",
			__FILE__, __LINE__, acl_last_serror());

	if (remote)
		acl_vstream_set_peer(stream, remote);
	if (local)
		acl_vstream_set_local(stream, local);
	/* when the stream is closed, the callback will be called
	 * to decrease the counter
	 */
	acl_vstream_add_close_handle(stream, decrease_counter_callback, NULL);

	ioctl_server_execute(h_ioctl, stream);
}

/* restart listening */

static void ioctl_restart_listen(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	ACL_VSTREAM *stream = (ACL_VSTREAM*) context;
	acl_assert(__h_ioctl);
	acl_msg_info("restart listen now!");
	acl_ioctl_enable_listen(__h_ioctl, stream, 0,
		ioctl_server_accept, __h_ioctl);
}

#ifdef MASTER_XPORT_NAME_PASS

/* ioctl_server_accept_pass - accept descriptor */

static void ioctl_server_accept_pass(int event_type, ACL_IOCTL *h_ioctl,
	ACL_VSTREAM *stream, void *context)
{
	const char  *myname = "ioctl_server_accept_pass";
	int     listen_fd = acl_vstream_fileno(stream);
	int     time_left = -1;
	int     fd;
	int     delay_listen = 0;
	int     i = 0;

	if (__sstreams == NULL) {
		acl_msg_info("Server stoping ...");
		return;
	}

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type(%d)",
			__FILE__, myname, __LINE__, event_type);

	/*
	 * Be prepared for accept() to fail because some other process already
	 * got the connection (the number of processes competing for clients
	 * is kept small, so this is not a "thundering herd" problem). If the
	 * accept() succeeds, be sure to disable non-blocking I/O, in order to
	 * minimize confusion.
	 */
	if (acl_var_ioctl_idle_limit > 0)
		time_left = (int) ((acl_ioctl_cancel_timer(h_ioctl,
			ioctl_server_timeout, h_ioctl) + 999999) / 1000000);
	else
		time_left = acl_var_ioctl_idle_limit;

	while (i++ < acl_var_ioctl_max_accept) {
		fd = PASS_ACCEPT(listen_fd);
		if (fd >= 0) {
			ioctl_server_wakeup(h_ioctl, fd, NULL, NULL);
			continue;
		}

		if (errno == EMFILE) {
			delay_listen = 1;
			acl_msg_warn("accept connection: %s", acl_last_serror());
		} else if (errno != EAGAIN && errno != EINTR) {
			acl_msg_warn("accept connection: %s, stoping ...",
				acl_last_serror());
			acl_ioctl_disable_readwrite(h_ioctl, stream);
			ioctl_server_abort(0, h_ioctl, stream, h_ioctl);
			return;
		}

		break;
	}

	if (delay_listen) {
		acl_ioctl_disable_readwrite(h_ioctl, stream);
		acl_ioctl_request_timer(h_ioctl, ioctl_restart_listen,
			stream, 2000000);
	} else
		acl_ioctl_enable_listen(h_ioctl, stream,
			0, ioctl_server_accept, context);

	if (time_left >= 0)
		acl_ioctl_request_timer(h_ioctl, ioctl_server_timeout,
			(void *) h_ioctl, (acl_int64) time_left * 1000000);
}

#endif

/* ioctl_server_accept_sock - accept client connection request */

static void ioctl_server_accept_sock(int event_type, ACL_IOCTL *h_ioctl,
	ACL_VSTREAM *stream, void *context)
{
	const char  *myname = "ioctl_serer_accept_inet";
	int     listen_fd = ACL_VSTREAM_SOCK(stream);
	int     time_left = -1, i = 0, delay_listen = 0, fd, sock_type;
	char    remote[64], local[64];

	if (__sstreams == NULL) {
		acl_msg_info("Server stoping ...");
		return;
	}

	if (event_type != ACL_EVENT_READ)
		acl_msg_fatal("%s, %s(%d): unknown event_type(%d)",
			__FILE__, myname, __LINE__, event_type);

	/*
	 * Be prepared for accept() to fail because some other process already
	 * got the connection (the number of processes competing for clients
	 * is kept small, so this is not a "thundering herd" problem). If the
	 * accept() succeeds, be sure to disable non-blocking I/O, in order to
	 * minimize confusion.
	 */
	if (acl_var_ioctl_idle_limit > 0)
		time_left = (int) ((acl_ioctl_cancel_timer(h_ioctl,
			ioctl_server_timeout, h_ioctl) + 999999) / 1000000);
	else
		time_left = acl_var_ioctl_idle_limit;

	while (i++ < acl_var_ioctl_max_accept) {
		fd = acl_accept(listen_fd, remote, sizeof(remote), &sock_type);
		if (fd < 0) {
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
			acl_ioctl_disable_readwrite(h_ioctl, stream);
			ioctl_server_abort(0, h_ioctl, stream, h_ioctl);
			return;
		}

	        /* 如果为 TCP 套接口，则设置 nodelay 选项以避免发送延迟现象 */
		if (sock_type == AF_INET)
			acl_tcp_set_nodelay(fd);
		if (acl_getsockname(fd, local, sizeof(local)) < 0)
			memset(local, 0, sizeof(local));
		ioctl_server_wakeup(h_ioctl, fd, remote, local);
	}

	if (delay_listen) {
		acl_ioctl_disable_readwrite(h_ioctl, stream);
		acl_ioctl_request_timer(h_ioctl, ioctl_restart_listen,
			stream, 2000000);
	} else
		acl_ioctl_enable_listen(h_ioctl, stream, 0,
			ioctl_server_accept, context);

	if (time_left > 0)
		acl_ioctl_request_timer(h_ioctl, ioctl_server_timeout,
			h_ioctl, (acl_int64) time_left * 1000000);
}

static void ioctl_server_init(const char *procname)
{
	const char *myname = "ioctl_server_init";
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
	acl_var_ioctl_pid = getpid();
	acl_var_ioctl_procname = acl_mystrdup(acl_safe_basename(procname));

	acl_var_ioctl_log_file = getenv("SERVICE_LOG");
	if (acl_var_ioctl_log_file == NULL) {
		acl_var_ioctl_log_file = acl_mystrdup("acl_master.log");
		acl_msg_warn("%s(%d)->%s: can't get SERVICE_LOG's env value,"
			" use %s log", __FILE__, __LINE__, myname,
			acl_var_ioctl_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_str_table(__conf_str_tab);

	acl_master_vars_init(acl_var_ioctl_buf_size, acl_var_ioctl_rw_timeout);
}

static void ioctl_server_open_log(void)
{
	/* first, close the master's log */
	master_log_close();

	/* second, open the service's log */
	acl_msg_open(acl_var_ioctl_log_file, acl_var_ioctl_procname);

	if (acl_var_ioctl_log_debug && *acl_var_ioctl_log_debug
		&& acl_var_ioctl_max_debug >= 100)
	{
		acl_debug_init2(acl_var_ioctl_log_debug,
			acl_var_ioctl_max_debug);
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
		acl_msg_fatal("%s(%d): argc(%d) invalid", __FILE__, __LINE__, argc);

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

/* acl_ioctl_server_main - the real main program */

void acl_ioctl_server_main(int argc, char **argv, ACL_IOCTL_SERVER_FN service, ...)
{
	const char *myname = "acl_ioctl_server_main";
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

	ACL_MASTER_SERVER_THREAD_INIT_FN thread_init_fn = NULL;
	ACL_MASTER_SERVER_THREAD_EXIT_FN thread_exit_fn = NULL;
	void  *thread_init_ctx = NULL;
	void  *thread_exit_ctx = NULL;

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
		ioctl_server_init(argv[0]);

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

		case ACL_MASTER_SERVER_THREAD_INIT:
			thread_init_fn = va_arg(ap, ACL_MASTER_SERVER_THREAD_INIT_FN);
			break;
		case ACL_MASTER_SERVER_THREAD_EXIT:
			thread_exit_fn = va_arg(ap, ACL_MASTER_SERVER_THREAD_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_THREAD_INIT_CTX:
			thread_init_ctx = va_arg(ap, void*);
			break;
		case ACL_MASTER_SERVER_THREAD_EXIT_CTX:
			thread_exit_ctx = va_arg(ap, void*);
			break;

		case ACL_MASTER_SERVER_PRE_INIT:
			pre_init = va_arg(ap, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_POST_INIT:
			post_init = va_arg(ap, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_EXIT:
			ioctl_server_onexit = va_arg(ap, ACL_MASTER_SERVER_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_SOLITARY:
			if (!alone)
				acl_msg_fatal("service %s requires a process"
					" limit of 1", service_name);
			break;
		case ACL_MASTER_SERVER_UNLIMITED:
			if (!zerolimit)
				acl_msg_fatal("service %s requires a process"
					" limit of 0", service_name);
			break;
		default:
			acl_msg_panic("%s: unknown argument type: %d", myname, key);
		}
	}
	va_end(ap);

	if (root_dir)
		root_dir = acl_var_ioctl_queue_dir;
	if (user_name)
		user_name = acl_var_ioctl_owner;

	/* If not connected to stdin, stdin must not be a terminal. */
	if (stream == 0 && isatty(STDIN_FILENO)) {
		printf("%s(%d), %s: do not run this command by hand\r\n",
			__FILE__, __LINE__, myname);
		exit (1);
	}

	/* Can options be required? */
	if (stream == 0) {
		if (transport == 0)
			acl_msg_fatal("no transport type specified");
		if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_INET) == 0) {
			ioctl_server_accept = ioctl_server_accept_sock;
			fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
		} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_UNIX) == 0) {
			ioctl_server_accept = ioctl_server_accept_sock;
			fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_UNIX;
		} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_SOCK) == 0) {
			ioctl_server_accept = ioctl_server_accept_sock;
			fdtype = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
#ifdef MASTER_XPORT_NAME_PASS
		} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_PASS) == 0) {
			ioctl_server_accept = ioctl_server_accept_pass;
			fdtype = ACL_VSTREAM_TYPE_LISTEN;
#endif
		} else
			acl_msg_fatal("unsupported transport type: %s", transport);
	}

	/*******************************************************************/

	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &ioctl_server_generation);
		if (acl_msg_verbose)
			acl_msg_info("process generation: %s (%o)",
				generation, ioctl_server_generation);
	}

	/*
	 * Traditionally, BSD select() can't handle ioctlple processes
	 * selecting on the same socket, and wakes up every process in
	 * select(). See TCP/IP Illustrated volume 2 page 532. We avoid
	 * select() collisions with an external lock file.
	 */

	/* Set up call-back info. */
	ioctl_server_service = service;
	ioctl_server_name = service_name;
	ioctl_server_argv = argv + optind;

	ioctl_init();

	/*******************************************************************/

	/* create event and call user's thread callback */
	if (strcasecmp(acl_var_ioctl_event_mode, "poll") == 0)
		event_mode = ACL_EVENT_POLL;
	else if (strcasecmp(acl_var_ioctl_event_mode, "kernel") == 0)
		event_mode = ACL_EVENT_KERNEL;
	else
		event_mode = ACL_EVENT_SELECT;

	__h_ioctl = acl_ioctl_create_ex(event_mode, acl_var_ioctl_max_threads,
		acl_var_ioctl_thread_idle_limit, acl_var_ioctl_delay_sec,
		acl_var_ioctl_delay_usec);
	acl_ioctl_ctl(__h_ioctl, ACL_IOCTL_CTL_THREAD_STACKSIZE,
		acl_var_ioctl_stacksize, ACL_IOCTL_CTL_END);

	if (thread_init_fn)
		acl_ioctl_ctl(__h_ioctl, ACL_IOCTL_CTL_INIT_FN, thread_init_fn,
			ACL_IOCTL_CTL_INIT_CTX, thread_init_ctx,
			ACL_IOCTL_CTL_END);
	if (thread_exit_fn)
		acl_ioctl_ctl(__h_ioctl, ACL_IOCTL_CTL_EXIT_FN, thread_exit_fn,
			ACL_IOCTL_CTL_EXIT_CTX, thread_exit_ctx,
			ACL_IOCTL_CTL_END);

	if (acl_var_ioctl_enable_dog)
		acl_ioctl_add_dog(__h_ioctl);

	/* 该函数内部创建事件引擎 */
	if (acl_ioctl_start(__h_ioctl) < 0)
		acl_msg_fatal("%s(%d): acl_ioctl_start error(%s)",
			myname, __LINE__, acl_last_serror());

	/*******************************************************************/

	/* Run pre-jail initialization. */
	if (chdir(acl_var_ioctl_queue_dir) < 0)
		acl_msg_fatal("chdir(\"%s\"): %s", acl_var_ioctl_queue_dir,
			acl_last_serror());

	if (pre_init)
		pre_init(ioctl_server_name);

	acl_chroot_uid(root_dir, user_name);
	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_ioctl_enable_core)
		acl_set_core_limit(0);
	ioctl_server_open_log();
	log_event_mode(event_mode);

	/*******************************************************************/

	/*
	 * Are we running as a one-shot server with the client connection on
	 * standard input? If so, make sure the output is written to stdout so as
	 * to satisfy common expectation.
	 */
	if (stream != 0) {
		if (post_init)
			post_init(ioctl_server_name);
		service(__h_ioctl, stream, ioctl_server_name, ioctl_server_argv);
		ioctl_server_exit();
	}

	/*******************************************************************/

	/*
	 * Running as a semi-resident server. Service connection requests.
	 * Terminate when we have serviced a sufficient number of clients, when
	 * no-one has been talking to us for a configurable amount of time, or
	 * when the master process terminated abnormally.
	 */

	if (acl_var_ioctl_idle_limit > 0)
		acl_ioctl_request_timer(__h_ioctl, ioctl_server_timeout,
			__h_ioctl, (acl_int64) acl_var_ioctl_idle_limit * 1000000);

	if (acl_var_ioctl_use_limit > 0)
		acl_ioctl_request_timer(__h_ioctl, ioctl_server_use_timer,
			__h_ioctl, (acl_int64) __use_limit_delay * 1000000);

	if (acl_var_ioctl_check_inter > 0) {
		ACL_EVENT *event = acl_ioctl_event(__h_ioctl);
		acl_event_set_check_inter(event, acl_var_ioctl_check_inter);
	}

	/*******************************************************************/

	/* socket count is as same listen_fd_count in parent process */

	__sstreams = (ACL_VSTREAM **)
		acl_mycalloc(__socket_count + 1, sizeof(ACL_VSTREAM *));
	for (i = 0; i < __socket_count + 1; i++)
		__sstreams[i] = NULL;

	i = 0;
	fd = ACL_MASTER_LISTEN_FD;
	for (; fd < ACL_MASTER_LISTEN_FD + __socket_count; fd++) {
		stream = acl_vstream_fdopen(fd, O_RDWR, acl_var_ioctl_buf_size,
			acl_var_ioctl_rw_timeout, fdtype);
		if (stream == NULL)
			acl_msg_fatal("%s(%d)->%s: stream null, fd = %d",
				__FILE__, __LINE__, myname, fd);

		acl_non_blocking(ACL_VSTREAM_SOCK(stream), ACL_NON_BLOCKING);
		acl_ioctl_enable_listen(__h_ioctl, stream, 0,
			ioctl_server_accept, NULL);
		acl_close_on_exec(ACL_VSTREAM_SOCK(stream), ACL_CLOSE_ON_EXEC);
		__sstreams[i++] = stream;
	}

	acl_ioctl_enable_read(__h_ioctl, ACL_MASTER_STAT_STREAM, 0,
		ioctl_server_abort, (void *) 0);
	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	/*******************************************************************/

	/* Run post-jail initialization. */
	if (post_init)
		post_init(ioctl_server_name);

	acl_msg_info("%s(%d), %s daemon started, log: %s",
		myname, __LINE__, argv[0], acl_var_ioctl_log_file);

	/*******************************************************************/

	while (1)
		sleep(1);

	/* not reached here */
	ioctl_server_exit();
}
#endif /* ACL_UNIX */
