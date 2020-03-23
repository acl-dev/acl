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
#include <pthread.h>

/* Utility library. */

#include "init/acl_init.h"
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
#include "net/acl_vstream_net.h"
#include "net/acl_access.h"
#include "event/acl_events.h"
#include "aio/acl_aio.h"

/* Application-specific */

#include "master/acl_master_flow.h"
#include "master/acl_master_proto.h"
#include "master/acl_aio_params.h"
#include "master/acl_server_api.h"
#include "master/acl_master_type.h"
#include "master/acl_master_conf.h"
#include "master_log.h"

int   acl_var_aio_pid;

int   acl_var_aio_buf_size;
int   acl_var_aio_rw_timeout;
int   acl_var_aio_in_flow_delay;
int   acl_var_aio_max_threads;
int   acl_var_aio_thread_idle_limit;
int   acl_var_aio_idle_limit;
int   acl_var_aio_delay_sec;
int   acl_var_aio_delay_usec;
int   acl_var_aio_daemon_timeout;
int   acl_var_aio_use_limit;
int   acl_var_aio_master_maxproc;
int   acl_var_aio_max_accept;
int   acl_var_aio_min_notify;
int   acl_var_aio_quick_abort;
int   acl_var_aio_enable_core;
int   acl_var_aio_disable_core_onexit;
int   acl_var_aio_accept_timer;
int   acl_var_aio_max_debug;
int   acl_var_aio_status_notify;

static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
        { ACL_VAR_AIO_BUF_SIZE, ACL_DEF_AIO_BUF_SIZE,
		&acl_var_aio_buf_size, 0, 0 },
        { ACL_VAR_AIO_RW_TIMEOUT, ACL_DEF_AIO_RW_TIMEOUT,
		&acl_var_aio_rw_timeout, 0, 0 },
        { ACL_VAR_AIO_IN_FLOW_DELAY, ACL_DEF_AIO_IN_FLOW_DELAY,
		&acl_var_aio_in_flow_delay, 0, 0 },
	{ ACL_VAR_AIO_MAX_THREADS, ACL_DEF_AIO_MAX_THREADS,
		&acl_var_aio_max_threads, 0, 0},
        { ACL_VAR_AIO_THREAD_IDLE_LIMIT, ACL_DEF_AIO_THREAD_IDLE_LIMIT,
		&acl_var_aio_thread_idle_limit, 0, 0 },
        { ACL_VAR_AIO_IDLE_LIMIT, ACL_DEF_AIO_IDLE_LIMIT,
		&acl_var_aio_idle_limit, 0, 0 },
        { ACL_VAR_AIO_DELAY_SEC, ACL_DEF_AIO_DELAY_SEC,
		&acl_var_aio_delay_sec, 0, 0 },
        { ACL_VAR_AIO_DELAY_USEC, ACL_DEF_AIO_DELAY_USEC,
		&acl_var_aio_delay_usec, 0, 0 },
        { ACL_VAR_AIO_DAEMON_TIMEOUT, ACL_DEF_AIO_DAEMON_TIMEOUT,
		&acl_var_aio_daemon_timeout, 0, 0 },
        { ACL_VAR_AIO_USE_LIMIT, ACL_DEF_AIO_USE_LIMIT,
		&acl_var_aio_use_limit, 0, 0 },
	{ ACL_VAR_AIO_MASTER_MAXPROC, ACL_DEF_AIO_MASTER_MAXPROC,
		&acl_var_aio_master_maxproc, 0, 0 },
	{ ACL_VAR_AIO_MAX_ACCEPT, ACL_DEF_AIO_MAX_ACCEPT,
		&acl_var_aio_max_accept, 0, 0 },
	{ ACL_VAR_AIO_MIN_NOTIFY, ACL_DEF_AIO_MIN_NOTIFY,
		&acl_var_aio_min_notify, 0, 0 },
	{ ACL_VAR_AIO_QUICK_ABORT, ACL_DEF_AIO_QUICK_ABORT,
		&acl_var_aio_quick_abort, 0, 0 },
	{ ACL_VAR_AIO_ENABLE_CORE, ACL_DEF_AIO_ENABLE_CORE,
		&acl_var_aio_enable_core, 0, 0 },
	{ ACL_VAR_AIO_DISABLE_CORE_ONEXIT, ACL_DEF_AIO_DISABLE_CORE_ONEXIT,
		&acl_var_aio_disable_core_onexit, 0, 0 },
	{ ACL_VAR_AIO_ACCEPT_TIMER, ACL_DEF_AIO_ACCEPT_TIMER,
		&acl_var_aio_accept_timer, 0, 0 },
	{ ACL_VAR_AIO_MAX_DEBUG, ACL_DEF_AIO_MAX_DEBUG,
		&acl_var_aio_max_debug, 0, 0 },
	{ ACL_VAR_AIO_STATUS_NOTIFY, ACL_DEF_AIO_STATUS_NOTIFY,
		&acl_var_aio_status_notify, 0, 0 },

        { 0, 0, 0, 0, 0 },
};

long long int acl_var_aio_core_limit;

static ACL_CONFIG_INT64_TABLE __conf_int64_tab[] = {
	{ ACL_VAR_AIO_CORE_LIMIT, ACL_DEF_AIO_CORE_LIMIT,
		&acl_var_aio_core_limit, 0, 0 },
        { 0, 0, 0, 0, 0 },
};

char *acl_var_aio_procname;
char *acl_var_aio_log_file;

char *acl_var_aio_queue_dir;
char *acl_var_aio_owner;
char *acl_var_aio_event_mode;
char *acl_var_aio_pid_dir;
char *acl_var_aio_access_allow;
char *acl_var_aio_accept_alone;
char *acl_var_aio_log_debug;
char *acl_var_aio_dispatch_addr;
char *acl_var_aio_dispatch_type;
char *acl_var_aio_deny_info;

static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
        { ACL_VAR_AIO_QUEUE_DIR, ACL_DEF_AIO_QUEUE_DIR,
		&acl_var_aio_queue_dir },
        { ACL_VAR_AIO_OWNER, ACL_DEF_AIO_OWNER,
		&acl_var_aio_owner },
	{ ACL_VAR_AIO_PID_DIR, ACL_DEF_AIO_PID_DIR,
		&acl_var_aio_pid_dir },
	{ ACL_VAR_AIO_ACCESS_ALLOW, ACL_DEF_AIO_ACCESS_ALLOW,
		&acl_var_aio_access_allow },
        { ACL_VAR_AIO_EVENT_MODE, ACL_DEF_AIO_EVENT_MODE,
		&acl_var_aio_event_mode },
	{ ACL_VAR_AIO_ACCEPT_ALONE, ACL_DEF_AIO_ACCEPT_ALONE,
		&acl_var_aio_accept_alone },
	{ ACL_VAR_AIO_LOG_DEBUG, ACL_DEF_AIO_LOG_DEBUG,
		&acl_var_aio_log_debug },
	{ ACL_VAR_AIO_DISPATCH_ADDR, ACL_DEF_AIO_DISPATCH_ADDR,
		&acl_var_aio_dispatch_addr },
	{ ACL_VAR_AIO_DISPATCH_TYPE, ACL_DEF_AIO_DISPATCH_TYPE,
		&acl_var_aio_dispatch_type },
	{ ACL_VAR_AIO_DENY_INFO, ACL_DEF_AIO_DENY_INFO,
		&acl_var_aio_deny_info },

        { 0, 0, 0 },
};

 /*
  * Global state.
  */
static int __event_mode;
static int __client_count = 0;
static int __use_count = 0;
static int __use_limit_delay = 1;
static int __socket_count = 1;
static int __listen_disabled = 0;
static int __aborting = 0;

static ACL_AIO *__h_aio = NULL;
static ACL_ASTREAM **__sstreams = NULL;

static time_t __last_closing_time = 0;
static pthread_mutex_t __closing_time_mutex;
static pthread_mutex_t __counter_mutex;

static ACL_AIO_SERVER_FN              __service_main;
static ACL_AIO_SERVER2_FN             __service2_main;
static ACL_MASTER_SERVER_EXIT_FN      __service_onexit;
static ACL_MASTER_SERVER_ON_LISTEN_FN __service_on_listen;
static ACL_MASTER_SERVER_SIGHUP_FN    __sighup_handler;
static char  *__service_name;
static char **__service_argv;
static void  *__service_ctx;
static void (*__service_accept) (ACL_ASTREAM *, void *);

static unsigned __aio_server_generation;
static char *__deny_info = NULL;
static char  __conf_file[1024];

static void dispatch_open(ACL_EVENT *event, ACL_AIO *aio);
static void dispatch_close(ACL_AIO *aio);

static void aio_init(void)
{
	if (pthread_mutex_init(&__closing_time_mutex, NULL) != 0)
		abort();
	if (pthread_mutex_init(&__counter_mutex, NULL) != 0)
		abort();

	__last_closing_time = time(NULL);
	__use_limit_delay = acl_var_aio_delay_sec > 1 ?
				acl_var_aio_delay_sec : 1;
}

static void lock_closing_time(void)
{
	if (pthread_mutex_lock(&__closing_time_mutex) != 0)
		abort();
}

static void unlock_closing_time(void)
{
	if (pthread_mutex_unlock(&__closing_time_mutex) != 0)
		abort();
}

static void lock_counter(void)
{
	if (pthread_mutex_lock(&__counter_mutex) != 0)
		abort();
}

static void unlock_counter(void)
{
	if (pthread_mutex_unlock(&__counter_mutex) != 0)
		abort();
}

static void update_closing_time(void)
{
	if (acl_var_aio_max_threads > 0)
		lock_closing_time();
	__last_closing_time = time(NULL);
	if (acl_var_aio_max_threads > 0)
		unlock_closing_time();
}

static time_t last_closing_time(void)
{
	time_t  last;

	if (acl_var_aio_max_threads > 0)
		lock_closing_time();
	last = __last_closing_time;
	if (acl_var_aio_max_threads > 0)
		unlock_closing_time();

	return last;
}

static void increase_client_counter(void)
{
	if (acl_var_aio_max_threads > 0)
		lock_counter();
	__client_count++;
	if (acl_var_aio_max_threads > 0)
		unlock_counter();
}

static void decrease_client_counter(void)
{
	if (acl_var_aio_max_threads > 0)
		lock_counter();
	__client_count--;
	if (acl_var_aio_max_threads > 0)
		unlock_counter();
}

static int get_client_count(void)
{
	int   n;

	if (acl_var_aio_max_threads > 0)
		lock_counter();
	n = __client_count;
	if (acl_var_aio_max_threads > 0)
		unlock_counter();

	return n;
}

const char *acl_aio_server_conf(void)
{
	return __conf_file;
}

ACL_EVENT *acl_aio_server_event()
{
	return acl_aio_event(__h_aio);
}

ACL_AIO* acl_aio_server_handle()
{
	return __h_aio;
}

ACL_ASTREAM **acl_aio_server_streams()
{
	if (__sstreams == NULL)
		acl_msg_warn("listen streams null!");
	return __sstreams;
}

void acl_aio_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, int delay)
{
	const char *myname = "acl_aio_server_request_timer";

	acl_aio_request_timer(__h_aio, timer_fn, arg,
		(acl_int64) delay * 1000000, 0);
	if (__h_aio == NULL)
		acl_msg_fatal("%s(%d)->%s: aio has not been inited",
			__FILE__, __LINE__, myname);
}

void acl_aio_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn, void *arg)
{
	const char *myname = "acl_aio_server_cancel_timer";

	if (__h_aio == NULL)
		acl_msg_fatal("%s(%d)->%s: aio has not been inited",
			__FILE__, __LINE__, myname);
	acl_aio_cancel_timer(__h_aio, timer_fn, arg);
}

static void disable_listen(void)
{
	int   i;

	if (__sstreams == NULL)
		return;
	for (i = 0; __sstreams[i] != NULL; i++) {
		acl_aio_disable_read(__sstreams[i]);
		acl_aio_iocp_close(__sstreams[i]);
		__sstreams[i] = NULL;
	}

	acl_myfree(__sstreams);
	__sstreams = NULL;
}

/* aio_server_exit - normal termination */

static void aio_server_exit(void)
{
#ifdef ACL_UNIX
	if (acl_var_aio_disable_core_onexit)
		acl_set_core_limit(0);
#endif
	if (__service_onexit)
		__service_onexit(__service_ctx);
	exit(0);
}

/* aio_server_timeout - idle time exceeded */

static void aio_server_timeout(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;
	time_t last, inter;
	int   n;

	n = get_client_count();

	/* if some fds not be closed, the timer should be reset again */
	if (n > 0 && acl_var_aio_idle_limit > 0) {                                         
		acl_aio_request_timer(aio, aio_server_timeout, (void *) aio,
			(acl_int64) acl_var_aio_idle_limit * 1000000, 0);
		return;
	}

	last  = last_closing_time();
	inter = time(NULL) - last;

	if (inter >= 0 && inter < acl_var_aio_idle_limit) {
		acl_aio_request_timer(aio, aio_server_timeout, (void *) aio,
			(acl_int64) (acl_var_aio_idle_limit - inter) * 1000000, 0);
		return;
	}

	if (acl_msg_verbose)
		acl_msg_info("idle timeout -- exiting");

	aio_server_exit();
}

/* aio_server_abort - terminate after abnormal master exit */

static void aio_server_abort(ACL_ASTREAM *astream, void *context acl_unused)
{
	const char *myname = "aio_server_abort";
	int   n;
	ACL_AIO *aio = acl_aio_handle(astream);

	if (__aborting)
		return;
	__aborting = 1;

	if (aio != __h_aio)
		acl_msg_fatal("%s(%d): aio invalid", myname, __LINE__);
	
	if (!__listen_disabled)
		__listen_disabled = 1;

	if (acl_var_aio_quick_abort) {
		acl_msg_info("master disconnect -- exiting");
		aio_server_exit();
	} else if ((n = get_client_count()) > 0) {
		/* set idle timeout to 1 second */
		acl_var_aio_idle_limit = 1;
		acl_aio_request_timer(__h_aio, aio_server_timeout, (void*) aio,
			(acl_int64) acl_var_aio_idle_limit * 1000000, 0);
	} else {
		acl_msg_info("master disconnect -- exiting");
		aio_server_exit();
	}
}

static void aio_server_read_abort(ACL_ASTREAM *astream, void *context,
	const char *data acl_unused, int dlen acl_unused)
{
	aio_server_abort(astream, context);
}

static void aio_server_use_timer(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;
	int   n;

	n = get_client_count();

	if (n > 0 || __use_count < acl_var_aio_use_limit) {
		acl_aio_request_timer(aio, aio_server_use_timer, (void *) aio,
			(acl_int64) __use_limit_delay * 1000000, 0);
		return;
	}

	acl_msg_info("use limit -- exiting");
	aio_server_exit();
}

int acl_aio_server_read(ACL_ASTREAM *astream, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context)
{
	const char *myname = "acl_aio_server_read";

	if (astream == NULL || notify_fn == NULL)
		acl_msg_fatal("%s(%d): input error", myname, __LINE__);

	acl_aio_ctl(astream, ACL_AIO_CTL_READ_HOOK_ADD, notify_fn, context,
		ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);
	acl_aio_read(astream);
	return 0;
}

int acl_aio_server_readn(ACL_ASTREAM *astream, int count, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context)
{
	const char *myname = "acl_aio_server_readn";

	if (astream == NULL || notify_fn == NULL || count <= 0)
		acl_msg_fatal("%s(%d): input error", myname, __LINE__);

	acl_aio_ctl(astream, ACL_AIO_CTL_READ_HOOK_ADD, notify_fn, context,
		ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);
	acl_aio_readn(astream, count);
	return 0;
}

int acl_aio_server_gets(ACL_ASTREAM *astream, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context)
{
	const char *myname = "acl_aio_server_gets";

	if (astream == NULL || notify_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	acl_aio_ctl(astream, ACL_AIO_CTL_READ_HOOK_ADD, notify_fn, context,
		ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);
	acl_aio_gets(astream);
	return 0;
}

int acl_aio_server_gets_nonl(ACL_ASTREAM *astream, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context)
{
	const char *myname = "acl_aio_server_gets_nonl";

	if (astream == NULL || notify_fn == NULL) 
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	acl_aio_ctl(astream, ACL_AIO_CTL_READ_HOOK_ADD, notify_fn, context,
		ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);
	acl_aio_gets_nonl(astream);
	return 0;
}

int acl_aio_server_writen(ACL_ASTREAM *astream, ACL_AIO_WRITE_FN notify_fn,
	void *context, const char *data, int dlen)
{
	const char *myname = "acl_aio_server_writen";

	if (astream == NULL || notify_fn == NULL || data == NULL || dlen <= 0)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	acl_aio_ctl(astream, ACL_AIO_CTL_WRITE_HOOK_ADD, notify_fn, context,
		ACL_AIO_CTL_END);
	acl_aio_writen(astream, data, dlen);
	return 0;
}

int acl_aio_server_vfprintf(ACL_ASTREAM *astream, ACL_AIO_WRITE_FN notify_fn,
	void *context, const char *fmt, va_list ap)
{
	const char *myname = "acl_aio_server_vfprintf";

	if (astream == NULL || notify_fn == NULL || fmt == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	acl_aio_ctl(astream, ACL_AIO_CTL_WRITE_HOOK_ADD, notify_fn, context,
		ACL_AIO_CTL_END);
	acl_aio_vfprintf(astream, fmt, ap);
	return 0;
}

int acl_aio_server_fprintf(ACL_ASTREAM *astream, ACL_AIO_WRITE_FN notify_fn,
	void *context, const char *fmt, ...)
{
	const char *myname = "acl_aio_server_fprintf";
	va_list ap;
	int   ret;

	if (astream == NULL || notify_fn == NULL || fmt == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	va_start(ap, fmt);
	ret = acl_aio_server_vfprintf(astream, notify_fn, context, fmt, ap);
	va_end(ap);

	return ret;
}

int acl_aio_server_connect(const char *saddr, int timeout,
	ACL_AIO_CONNECT_FN connect_fn, void *context)
{
	const char *myname = "acl_aio_server_connect";
	ACL_ASTREAM *astream;

	if (saddr == NULL || connect_fn == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	astream = acl_aio_connect(__h_aio, saddr, timeout);
	if (astream == NULL)
		return -1;
	acl_aio_ctl(astream, ACL_AIO_CTL_CONNECT_HOOK_ADD, connect_fn, context,
		ACL_AIO_CTL_TIMEOUT, timeout, ACL_AIO_CTL_END);
	return 0;
}

void acl_aio_server_on_close(ACL_ASTREAM *stream acl_unused)
{
	update_closing_time();
	decrease_client_counter();
}

/* server_wakeup - wake up application in main thread */

static void disconnect_callback(ACL_VSTREAM *vs acl_unused, void *ctx)
{
	ACL_ASTREAM *as = (ACL_ASTREAM*) ctx;

	acl_aio_server_on_close(as);
}

static void server_wakeup(ACL_AIO *aio, int fd)
{
	const char *myname = "server_wakeup";
	char  addr[256], *ptr;

	acl_non_blocking(fd, ACL_NON_BLOCKING);
	acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);

	increase_client_counter();

	if (acl_getpeername(fd, addr, sizeof(addr)) < 0) {
		if (acl_msg_verbose)
			acl_msg_warn("%s, %s(%d): get socket's addr error: %s",
				__FILE__, myname, __LINE__, acl_last_serror());
		acl_socket_close(fd);
		return;
	}

	ptr = strchr(addr, ':');
	if (ptr)
		*ptr = 0;

	if (!acl_access_permit(addr)) {
		if (acl_msg_verbose)
			acl_msg_warn("%s, %s(%d): addr(%s) be denied",
				__FILE__, myname, __LINE__, addr);

		if (__deny_info && *__deny_info) {
			if (write(fd, __deny_info, strlen(__deny_info)) > 0
				&& write(fd, "\r\n", 2) > 0)
			{
				/* do nothing, just avoid compile warning */
			}
		}

		acl_socket_close(fd);
		return;
	}

	__use_count++;

	if (__service_main != NULL) {
		ACL_VSTREAM *vs;
		ACL_ASTREAM *as;

		vs = acl_vstream_fdopen(fd, O_RDWR, acl_var_aio_buf_size,
			0, ACL_VSTREAM_TYPE_SOCK);
		acl_vstream_set_peer(vs, addr);
		acl_getsockname(fd, addr, sizeof(addr));
		acl_vstream_set_local(vs, addr);

		as = acl_aio_open(aio, vs);
		acl_vstream_add_close_handle(vs, disconnect_callback, as);

		__service_main(as, __service_ctx);
	} else if (__service2_main != NULL)
		__service2_main(fd, __service_ctx);
	else
		acl_msg_fatal("%s(%d), %s: service_callback null",
			__FILE__, __LINE__, myname);
}

static void dummy(void *ptr acl_unused)
{
}

static void free_tls(void *ptr)
{
	acl_myfree(ptr);
}

static void *__tls = NULL;
#ifndef HAVE_NO_ATEXIT
static void main_free_tls(void)
{
	if (__tls) {
		acl_myfree(__tls);
		__tls = NULL;
	}
}
#endif

static acl_pthread_key_t  once_key;
static void once_init(void)
{
	if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
		acl_pthread_key_create(&once_key, dummy);
#ifndef HAVE_NO_ATEXIT
		atexit(main_free_tls);
#endif
	} else
		acl_pthread_key_create(&once_key, free_tls);
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

static void *tls_alloc(size_t len)
{
	void *ptr;

	(void) acl_pthread_once(&once_control, once_init);
	ptr = acl_pthread_getspecific(once_key);
	if (ptr == NULL) {
		ptr = acl_mymalloc(len);
		acl_pthread_setspecific(once_key, ptr);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self())
			__tls = ptr;
	}
	return ptr;
}

/* restart listening */

static void restart_listen(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	ACL_ASTREAM *stream = (ACL_ASTREAM*) context;
	acl_msg_info("restart listen now!");
	acl_aio_listen(stream);
}

#ifdef MASTER_XPORT_NAME_PASS

/* aio_server_accept_pass - accept descriptor */

static void aio_server_accept_pass(ACL_ASTREAM *astream, void *context)
{
	const char *myname = "aio_server_accept_pass";
	ACL_VSTREAM *vstream = acl_aio_vstream(astream);
	int     listen_fd = acl_vstream_fileno(vstream);
	ACL_AIO *aio = (ACL_AIO *) context;
	int     time_left = -1;
	int     fd;
	int    *fds;
	int     i, j;
	int     delay_listen = 0;

	/*
	 * Be prepared for accept() to fail because some other process already
	 * got the connection (the number of processes competing for clients
	 * is kept small, so this is not a "thundering herd" problem). If the
	 * accept() succeeds, be sure to disable non-blocking I/O, in order to
	 * minimize confusion.
	 */
	if (acl_var_aio_idle_limit > 0)
		time_left = (int) ((acl_aio_cancel_timer(aio, aio_server_timeout,
					(void *) aio) + 999999) / 1000000);
	else
		time_left = acl_var_aio_idle_limit;

	fds = (int*) tls_alloc(sizeof(int) * acl_var_aio_max_accept);

	for (i = 0; i < acl_var_aio_max_accept; i++) {
		fd = PASS_ACCEPT(listen_fd);
		if (fd >= 0)
			fds[i] = fd;
		else if (errno == EMFILE) {
			delay_listen = 1;
			acl_msg_warn("%s: accept error %s",
				myname, acl_last_serror());
			break;
		} else if (errno == EAGAIN || errno == EINTR)
			break;
		else
			acl_msg_fatal("%s: accept error %s",
				myname, acl_last_serror());
	}

	if (acl_var_aio_status_notify && acl_var_aio_master_maxproc > 1
		&& i >= acl_var_aio_min_notify && acl_master_notify(
			acl_var_aio_pid, __aio_server_generation,
			ACL_MASTER_STAT_TAKEN) < 0)
	{
		aio_server_abort(astream, NULL);
	}

	for (j = 0; j < i; j++)
		server_wakeup(aio, fds[j]);

	if (acl_var_aio_status_notify && acl_var_aio_master_maxproc > 1
		&& i >= acl_var_aio_min_notify && acl_master_notify(
			acl_var_aio_pid, __aio_server_generation,
			ACL_MASTER_STAT_AVAIL) < 0)
	{
		aio_server_abort(astream, NULL);
	}

	if (delay_listen) {
		acl_aio_disable_read(astream);
		acl_aio_request_timer(aio, restart_listen, astream, 2000000, 0);
	}

	if (time_left > 0)
		acl_aio_request_timer(aio, aio_server_timeout, (void *) aio,
			(acl_int64) time_left * 1000000, 0);
}

#endif

/* aio_server_accept_sock2 - accept client connection request */

static int aio_server_accept_sock2(ACL_ASTREAM *astream, ACL_AIO *aio)
{
	const char *myname = "aio_serer_accept_sock2";
	ACL_VSTREAM *vstream = acl_aio_vstream(astream);
	int    listen_fd = ACL_VSTREAM_SOCK(vstream);
	int    fd, *fds, i, j, delay_listen = 0, sock_type;

	fds = (int*) tls_alloc(sizeof(int) * acl_var_aio_max_accept);

	/*
	 * Be prepared for accept() to fail because some other process already
	 * got the connection (the number of processes competing for clients
	 * is kept small, so this is not a "thundering herd" problem). If the
	 * accept() succeeds, be sure to disable non-blocking I/O, in order to
	 * minimize confusion.
	 */

	for (i = 0; i < acl_var_aio_max_accept; i++) {
		fd = acl_accept(listen_fd, NULL, 0, &sock_type);
#ifdef ACL_WINDOWS
		if (fd != ACL_SOCKET_INVALID) {
#else
		if (fd >= 0) {
#endif
			/* TCP 连接避免发送延迟现象 */
#ifdef AF_INET6
			if (sock_type == AF_INET || sock_type == AF_INET6)
#else
			if (sock_type == AF_INET)
#endif
				acl_tcp_set_nodelay(fd);
			fds[i] = fd;
		} else if (errno == EMFILE) {
			delay_listen = 1;
			acl_msg_warn("%s(%d), %s: accept connection: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
		} else if (errno == EAGAIN || errno == EINTR)
			break;
		else
			acl_msg_fatal("%s(%d), %s: accept connection: %s",
				__FILE__, __LINE__, myname, acl_last_serror());
	}

	if (acl_var_aio_status_notify && acl_var_aio_master_maxproc > 1
		&& i >= acl_var_aio_min_notify && acl_master_notify(
			acl_var_aio_pid, __aio_server_generation,
			ACL_MASTER_STAT_TAKEN) < 0)
	{
		aio_server_abort(astream, NULL);
	}

	for (j = 0; j < i; j++)
		server_wakeup(aio, fds[j]);

	if (acl_var_aio_status_notify && acl_var_aio_master_maxproc > 1
		&& i >= acl_var_aio_min_notify && acl_master_notify(
			acl_var_aio_pid, __aio_server_generation,
			ACL_MASTER_STAT_AVAIL) < 0)
	{
		aio_server_abort(astream, NULL);
	}

	if (delay_listen) {
		acl_aio_disable_read(astream);
		acl_aio_request_timer(aio, restart_listen, astream, 2000000, 0);
	}

	return i;
}

static void aio_server_accept_timer(int type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	ACL_ASTREAM *astream = (ACL_ASTREAM*) context;
	ACL_AIO *aio = acl_aio_handle(astream);

	(void) aio_server_accept_sock2(astream, aio);
	acl_aio_request_timer(aio, aio_server_accept_timer,
		astream, (acl_int64) acl_var_aio_accept_timer * 1000000, 0);
}

static void aio_server_accept_sock(ACL_ASTREAM *astream, void *context)
{
	ACL_AIO *aio = (ACL_AIO *) context;
	int    time_left = -1;

	if (acl_var_aio_idle_limit > 0)
		time_left = (int) ((acl_aio_cancel_timer(aio, aio_server_timeout,
			(void *) aio) + 999999) / 1000000);
	else
		time_left = acl_var_aio_idle_limit;

	(void) aio_server_accept_sock2(astream, aio);

	if (time_left > 0)
		acl_aio_request_timer(aio, aio_server_timeout,
			(void *) aio, (acl_int64) time_left * 1000000, 0);
}

/*==========================================================================*/

static void dispatch_connect_timer(int type acl_unused,
	ACL_EVENT *event, void *ctx)
{
	ACL_AIO *aio = (ACL_AIO*) ctx;

	dispatch_open(event, aio);
}

static ACL_VSTREAM *__dispatch_conn = NULL;

static int dispatch_report(void)
{
	const char *myname = "dispatch_report";
	char  buf[256];
	int   n;

	if (__dispatch_conn == NULL) {
		acl_msg_warn("%s(%d), %s: dispatch connection not available",
			__FUNCTION__, __LINE__, myname);
		return -1;
	}

	n = get_client_count();
	snprintf(buf, sizeof(buf), "count=%d&used=%d&pid=%u&type=%s\r\n",
		n, __use_count, (unsigned) getpid(),
		acl_var_aio_dispatch_type);

	if (acl_vstream_writen(__dispatch_conn, buf, strlen(buf))
		== ACL_VSTREAM_EOF)
	{
		acl_msg_warn("%s(%d), %s: write to master_dispatch(%s) failed",
			__FUNCTION__, __LINE__, myname,
			acl_var_aio_dispatch_addr);

		return -1;
	}

	return 0;
}

static void dispatch_timer(int type acl_unused, ACL_EVENT *event, void *ctx)
{
	if (dispatch_report() == 0)
		acl_event_request_timer(event, dispatch_timer, ctx, 1000000, 0);
}

static void dispatch_receive(int event_type acl_unused, ACL_EVENT *event,
	ACL_VSTREAM *conn, void *context)
{
	const char *myname = "dispatch_receive";
	ACL_AIO *aio = (ACL_AIO*) context;
	char  buf[256];
	int   fd = -1, ret;

	if (conn != __dispatch_conn)
		acl_msg_fatal("%s(%d), %s: conn invalid",
			__FUNCTION__, __LINE__, myname);

	ret = acl_read_fd(ACL_VSTREAM_SOCK(conn), buf, sizeof(buf) - 1, &fd);
	if (ret <= 0 || fd < 0) {
		acl_msg_warn("%s(%d), %s: read from master_dispatch(%s) error",
			__FUNCTION__, __LINE__, myname,
			acl_var_aio_dispatch_addr);

		acl_event_disable_read(event, conn);
		acl_vstream_close(conn);
		__dispatch_conn = NULL;

		acl_event_request_timer(event, dispatch_connect_timer,
			aio, 1000000, 0);

		return;
	}

	ret = acl_getsocktype(fd);
#ifdef AF_INET6
	if (ret == AF_INET || ret == AF_INET6)
#else
	if (ret == AF_INET)
#endif
		acl_tcp_set_nodelay(fd);

	/* begin handle one client connection same as accept */

	server_wakeup(aio, fd);
}

static void dispatch_close(ACL_AIO *aio)
{
	if (__dispatch_conn) {
		ACL_EVENT *event = acl_aio_event(aio);

		acl_event_disable_readwrite(event, __dispatch_conn);
		acl_event_cancel_timer(event, dispatch_connect_timer, aio);
		acl_event_cancel_timer(event, dispatch_timer, aio);
		acl_vstream_close(__dispatch_conn);
		__dispatch_conn = NULL;
	}
}

static void dispatch_open(ACL_EVENT *event, ACL_AIO *aio)
{
	const char *myname = "dispatch_open";

	if (__aborting) {
		acl_msg_info("%s(%d), %s: master disconnect -- aborting",
			__FILE__, __LINE__, myname);
		return;
	}

	if (!acl_var_aio_dispatch_addr || !*acl_var_aio_dispatch_addr)
	{
		acl_msg_warn("%s(%d), %s: acl_var_aio_dispatch_addr null",
			__FUNCTION__, __LINE__, myname);
		return;
	}

	__dispatch_conn = acl_vstream_connect(acl_var_aio_dispatch_addr,
			ACL_BLOCKING, 0, 0, 4096);

	if (__dispatch_conn == NULL) {
		acl_msg_warn("connect master_dispatch(%s) failed",
			acl_var_aio_dispatch_addr);
		acl_event_request_timer(event, dispatch_connect_timer,
			aio, 1000000, 0);
	} else if (dispatch_report() == 0) {
		acl_event_enable_read(event, __dispatch_conn, 0,
			dispatch_receive, aio);
		acl_event_request_timer(event, dispatch_timer,
			aio, 1000000, 0);
	} else
		acl_event_request_timer(event, dispatch_connect_timer,
			aio, 1000000, 0);
}

/*==========================================================================*/

static void aio_server_init(const char *procname)
{
	const char *myname = "aio_server_init";
	static int  inited = 0;

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
	acl_var_aio_pid = getpid();
	acl_var_aio_procname = acl_mystrdup(acl_safe_basename(procname));

	acl_var_aio_log_file = getenv("SERVICE_LOG");
	if (acl_var_aio_log_file == NULL) {
		acl_var_aio_log_file = acl_mystrdup("acl_master.log");
		acl_msg_warn("%s(%d), %s: no SERVICE_LOG env, use %s log",
			__FILE__, __LINE__, myname, acl_var_aio_log_file);
	}

	/* 获得本服务器框架所需要的配置参数 */

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_int64_table(__conf_int64_tab);
	acl_get_app_conf_str_table(__conf_str_tab);
}

static void open_service_log(void)
{
	/* first, close the master's log */
	master_log_close();

	/* second, open the service's log */
	acl_msg_open(acl_var_aio_log_file, acl_var_aio_procname);

	if (acl_var_aio_log_debug && *acl_var_aio_log_debug
		&& acl_var_aio_max_debug >= 100)
	{
		acl_debug_init2(acl_var_aio_log_debug, acl_var_aio_max_debug);
	}
}

static void usage(int argc, char *argv[])
{
	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc: %d", __FILE__, __LINE__, argc);

	acl_msg_info("usage: %s -H[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -f conf_file", argv[0]);
}

/* 创建异步IO */

static ACL_AIO *create_aio(int *event_mode)
{
	ACL_AIO *aio;

	if (strcasecmp(acl_var_aio_event_mode, "poll") == 0)
		*event_mode = ACL_EVENT_POLL;
	else if (strcasecmp(acl_var_aio_event_mode, "kernel") == 0)
		*event_mode = ACL_EVENT_KERNEL;
	else
		*event_mode = ACL_EVENT_SELECT;

	aio = acl_aio_create(*event_mode);
	acl_aio_set_delay_sec(aio, acl_var_aio_delay_sec);
	acl_aio_set_delay_usec(aio, acl_var_aio_delay_usec);
	acl_aio_set_keep_read(aio, 1);
	return aio;
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

/* 创建定时器 */

static void create_timer(ACL_AIO *aio, int use_limit_delay)
{
	/*
	 * Running as a semi-resident server. Service connection requests.
	 * Terminate when we have serviced a sufficient number of clients, when
	 * no-one has been talking to us for a configurable amount of time, or
	 * when the master process terminated abnormally.
	 */
	if (acl_var_aio_idle_limit > 0)
		acl_aio_request_timer(aio, aio_server_timeout,
			aio, (acl_int64) acl_var_aio_idle_limit * 1000000, 0);
	if (acl_var_aio_use_limit > 0)
		acl_aio_request_timer(aio, aio_server_use_timer,
			aio, (acl_int64) use_limit_delay * 1000000, 0);
}

/* 创建监听者 */

static ACL_ASTREAM **create_listener(ACL_AIO *aio, int event_mode acl_unused,
	const char *transport, int socket_count)
{
	const char *myname = "create_listener";
	int   i, fd, type = 0;
	ACL_VSTREAM *vs;
	ACL_ASTREAM *as, **sstreams;

	/* socket count is as same listen_fd_count in parent process */

	sstreams = (ACL_ASTREAM **) acl_mycalloc(socket_count + 1,
			sizeof(ACL_ASTREAM *));
	for (i = 0; i < socket_count + 1; i++)
		sstreams[i] = NULL;

	/* 选择连接接收接口 */

	if (transport == 0)
		acl_msg_fatal("%s: no transport type specified", myname);
	if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_INET) == 0) {
		__service_accept = aio_server_accept_sock;
		type = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_UNIX) == 0) {
		__service_accept = aio_server_accept_sock;
		type = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_UNIX;
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_SOCK) == 0) {
		__service_accept = aio_server_accept_sock;
		type = ACL_VSTREAM_TYPE_LISTEN | ACL_VSTREAM_TYPE_LISTEN_INET;
#ifdef MASTER_XPORT_NAME_PASS
	} else if (strcasecmp(transport, ACL_MASTER_XPORT_NAME_PASS) == 0) {
		__service_accept = aio_server_accept_pass;
		type = ACL_VSTREAM_TYPE_LISTEN;
#endif
	} else
		acl_msg_fatal("%s: unsupported transport type: %s",
			myname, transport);

	for (i = 0, fd = ACL_MASTER_LISTEN_FD;
		fd < ACL_MASTER_LISTEN_FD + socket_count; fd++)
	{
		/* 打开监听数据流 */
		vs = acl_vstream_fdopen(fd, O_RDWR, acl_var_aio_buf_size,
			acl_var_aio_rw_timeout, type);
		acl_assert(vs);

		acl_non_blocking(ACL_VSTREAM_SOCK(vs), ACL_NON_BLOCKING);
		acl_close_on_exec(ACL_VSTREAM_SOCK(vs), ACL_CLOSE_ON_EXEC);

		/* 打开异步数据流 */
		as = acl_aio_open(aio, vs);
		acl_assert(as);

		/* 若采用子线程单独 accept 连接，则 __service_accept 在子线程
		 * 的异步事件循环中运行，否则在主线程的异步事件循环中运行
		 */
		acl_aio_ctl(as, ACL_AIO_CTL_LISTEN_FN, __service_accept, 
			ACL_AIO_CTL_CTX, aio, ACL_AIO_CTL_END);

		/* 设置异步监听 */
		acl_aio_listen(as);
		if (__service_on_listen)
			__service_on_listen(__service_ctx, vs);
		sstreams[i++] = as;

		if (acl_var_aio_accept_timer <= 0)
			continue;

		/* 为了保证 accept 的优先级，可以设置接收定时器 */

		acl_aio_request_timer(aio, aio_server_accept_timer,
			as, (acl_int64) acl_var_aio_accept_timer * 1000000, 0);
	}

	return sstreams;
}

/* 进程间通信设置 */

static void setup_ipc(ACL_AIO *aio)
{
	ACL_VSTREAM *stream = acl_vstream_fdopen(ACL_MASTER_STATUS_FD,
			O_RDWR, 8192, 0, ACL_VSTREAM_TYPE_SOCK);
	ACL_ASTREAM *stat_astream = acl_aio_open(aio, stream);

	acl_aio_ctl(stat_astream,
		ACL_AIO_CTL_READ_HOOK_ADD, aio_server_read_abort, NULL,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, aio_server_abort, NULL,
		ACL_AIO_CTL_END);
	acl_aio_read(stat_astream);

	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);
}

/* 开始进入事件循环过程 */

static void run_loop(const char *procname)
{
	ACL_VSTRING *buf = acl_vstring_alloc(128);

	acl_msg_info("%s: starting...(accept_alone: %s)",
		procname, acl_var_aio_accept_alone);

	if (acl_msg_verbose)
		acl_msg_info("%s(%d): daemon started, log = %s",
			acl_var_aio_procname, __LINE__, acl_var_aio_log_file);

	/*
	 * Traditionally, BSD select() can't handle aiople processes
	 * selecting on the same socket, and wakes up every process
	 * in select().
	 * See TCP/IP Illustrated volume 2 page 532. We avoid select()
	 * collisions with an external lock file.
	 */

	acl_server_sighup_setup();
	acl_server_sigterm_setup();

	while (1) {
		if (acl_var_aio_max_threads == 0)  /* single thread mode */
			acl_aio_loop(__h_aio);
		else  /* multi-threads mode */
			sleep(1);

		if (acl_var_server_gotsighup && __sighup_handler) {
			acl_var_server_gotsighup = 0;
			if (__sighup_handler(__service_ctx, buf) < 0)
				acl_master_notify(acl_var_aio_pid,
					__aio_server_generation,
					ACL_MASTER_STAT_SIGHUP_ERR);
			else
				acl_master_notify(acl_var_aio_pid,
					__aio_server_generation,
					ACL_MASTER_STAT_SIGHUP_OK);
		}

		if (__listen_disabled == 1) {
			__listen_disabled = 2;

			/* 该进程不再负责监听，防止 acl_master 主进程
			 * 无法正常重启
			 */
			disable_listen();
			dispatch_close(__h_aio);
		}
	}

    /* not reached here */
    
	/* acl_vstring_free(buf); */
	/* aio_server_exit(); */
}

/* acl_aio_server_main - the real main program */

static void server_main(int argc, char **argv, va_list ap)
{
	const char *myname = "acl_aio_server_main";
	ACL_MASTER_SERVER_INIT_FN pre_init = 0;
	ACL_MASTER_SERVER_INIT_FN post_init = 0;
	char   *service_name = acl_mystrdup(acl_safe_basename(argv[0]));
	char   *root_dir = 0, *user_name = 0;
	char   *transport = 0, *generation;
	int     c;

	/*******************************************************************/

	/* If not connected to stdin, stdin must not be a terminal. */
	if (isatty(STDIN_FILENO)) {
		printf("%s(%d), %s: do not run this command by hand\r\n",
			__FILE__, __LINE__, myname);
		exit (1);
	}

	/*******************************************************************/

	aio_init();  /* 初始化 */

	/* 在子进程切换用户身份之前，先用 acl_master 的日志句柄记日志 */
	master_log_open(argv[0]);

	/*******************************************************************/

	/*
	 * Pick up policy settings from master process. Shut up error
	 * messages to stderr, because no-one is going to see them.
	 */

#ifdef ACL_LINUX
	opterr = 0;
	optind = 0;
	optarg = 0;
#endif

	__conf_file[0] = 0;

	while ((c = getopt(argc, argv, "Hcn:o:s:t:uf:")) > 0) {
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
			transport = optarg;
			break;
		default:
			break;
		}
	}

	aio_server_init(argv[0]);

	if (__conf_file[0] == 0)
		acl_msg_fatal("%s(%d), %s: need \"-f pathname\"",
			__FILE__, __LINE__, myname);
	else if (acl_msg_verbose)
		acl_msg_info("%s(%d), %s: configure file = %s", 
			__FILE__, __LINE__, myname, __conf_file);

	/* Application-specific initialization. */

	while ((c = va_arg(ap, int)) != 0) {
		switch (c) {
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
		case ACL_MASTER_SERVER_DENY_INFO:
			__deny_info = acl_mystrdup(va_arg(ap, const char*));
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
			__service_onexit =
				va_arg(ap, ACL_MASTER_SERVER_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_ON_LISTEN:
			__service_on_listen =
				va_arg(ap, ACL_MASTER_SERVER_ON_LISTEN_FN);
			break;
		case ACL_MASTER_SERVER_SIGHUP:
			__sighup_handler =
				va_arg(ap, ACL_MASTER_SERVER_SIGHUP_FN);
			break;
		default:
			acl_msg_warn("%s: unknown argument: %d", myname, c);
		}
	}

	va_end(ap);

	if (root_dir)
		root_dir = acl_var_aio_queue_dir;
	if (user_name)
		user_name = acl_var_aio_owner;

	if (__deny_info == NULL)
		__deny_info = acl_var_aio_deny_info;

	/*
	 * Retrieve process generation from environment.
	 */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &__aio_server_generation);
		if (acl_msg_verbose)
			acl_msg_info("process generation: %s (%o)",
				generation, __aio_server_generation);
	}

	/* Set up call-back info.  */

	__service_name = service_name;
	__service_argv = argv + optind;

	__h_aio = create_aio(&__event_mode);  /* 创建异步IO引擎 */

	/* change to given directory */
	if (chdir(acl_var_aio_queue_dir) < 0)
		acl_msg_fatal("chdir(\"%s\"): %s", acl_var_aio_queue_dir,
			acl_last_serror());

	/* 增加 ip 地址限制 */
	if (acl_var_aio_access_allow && *acl_var_aio_access_allow)
		acl_access_add(acl_var_aio_access_allow, ", \t", ":");

	/* Run pre-jail initialization. */
	if (pre_init)
		pre_init(__service_ctx);

	acl_chroot_uid(root_dir, user_name);  /* 切换用户身份 */
	open_service_log();  /* 打开本进程自己的日志 */

#ifdef ACL_UNIX
	/* 设置子进程运行环境，允许产生 core 文件 */
	if (acl_var_aio_enable_core && acl_var_aio_core_limit != 0) {
		acl_set_core_limit(acl_var_aio_core_limit);
	}
#endif

	log_event_mode(__event_mode);  /* 将事件模式记入日志中 */

	create_timer(__h_aio, __use_limit_delay);  /* 创建定时器 */
	__sstreams = create_listener(__h_aio, __event_mode,
		transport, __socket_count);  /* 创建监听者 */
	setup_ipc(__h_aio);  /* 安装进程间通信的通道 */

	/* Run post-jail initialization. */
	if (post_init)
		post_init(__service_ctx);

#ifdef ACL_LINUX
	/* notify master that child started ok */
	if (generation)
		acl_master_notify(acl_var_aio_pid, __aio_server_generation,
			ACL_MASTER_STAT_START_OK);
#endif

	if (acl_var_aio_dispatch_addr && *acl_var_aio_dispatch_addr)
		dispatch_open(acl_aio_server_event(), __h_aio);

	run_loop(argv[0]);  /* 进入事件主循环过程 */
}

void acl_aio_server_main(int argc, char **argv, ACL_AIO_SERVER_FN service,...)
{
	va_list ap;

	__service_main = service;

	va_start(ap, service);
	/* ap 将在 server_mainn 中收尾 */
	server_main(argc, argv, ap);
}

void acl_aio_server2_main(int argc, char **argv, ACL_AIO_SERVER2_FN service,...)
{
	va_list ap;

	__service2_main = service;

	va_start(ap, service);
	/* ap 将在 server_mainn 中收尾 */
	server_main(argc, argv, ap);
}

/****************************************************************************/

static ACL_AIO_RUN_FN __app_run;

static void service_adapter(ACL_ASTREAM *astream, void *ctx)
{
	const char *myname = "service_adapter";

	if (__app_run) {
		if (__app_run(astream, ctx) != 0)
			acl_aio_iocp_close(astream);
	} else
		acl_msg_error("%s(%d): __app_run null", myname, __LINE__);
}

void acl_aio_app_main(int argc, char *argv[], ACL_AIO_RUN_FN run_fn,
	void *run_ctx, ...)                                         
{
	va_list ap;

	va_start(ap, run_ctx);
	__service_main = service_adapter;
	__app_run = run_fn;
	__service_ctx = run_ctx;

	server_main(argc, argv, ap);
}

static ACL_AIO_RUN2_FN __app2_run;

static void service_adapter2(ACL_SOCKET fd, void *ctx)
{
	const char *myname = "service_adapter";

	if (__app2_run) {
		if (__app2_run(fd, ctx) != 0)
			acl_socket_close(fd);
	} else
		acl_msg_error("%s(%d): __app2_run null", myname, __LINE__);
}

void acl_aio_app2_main(int argc, char *argv[], ACL_AIO_RUN2_FN run2_fn,
	void *run_ctx, ...)
{
	va_list ap;

	va_start(ap, run_ctx);
	__service2_main = service_adapter2;
	__app2_run = run2_fn;
	__service_ctx = run_ctx;

	server_main(argc, argv, ap);
}

#endif /* ACL_UNIX */
#endif /* ACL_CLIENT_ONLY */
