#ifndef	ACL_AIO_PARAMS_INCLUDE_H
#define	ACL_AIO_PARAMS_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef ACL_UNIX

extern int   acl_var_aio_pid;
extern char *acl_var_aio_procname;
extern char *acl_var_aio_log_file;

#define	ACL_VAR_AIO_BUF_SIZE		"aio_buf_size"
#define	ACL_DEF_AIO_BUF_SIZE		81920
extern int   acl_var_aio_buf_size;

#define	ACL_VAR_AIO_RW_TIMEOUT		"aio_rw_timeout"
#define	ACL_DEF_AIO_RW_TIMEOUT		30
extern int   acl_var_aio_rw_timeout;

#define	ACL_VAR_AIO_IN_FLOW_DELAY	"aio_in_flow_delay"
#define	ACL_DEF_AIO_IN_FLOW_DELAY	1
extern int   acl_var_aio_in_flow_delay;

#define	ACL_VAR_AIO_MAX_THREADS		"aio_max_threads"
#define	ACL_DEF_AIO_MAX_THREADS		0
extern int   acl_var_aio_max_threads;

#define	ACL_VAR_AIO_THREAD_IDLE_LIMIT	"aio_thread_idle_limit"
#define	ACL_DEF_AIO_THREAD_IDLE_LIMIT	0
extern int   acl_var_aio_thread_idle_limit;

#define	ACL_VAR_AIO_IDLE_LIMIT		"aio_idle_limit"
#define	ACL_DEF_AIO_IDLE_LIMIT		0
extern int   acl_var_aio_idle_limit;

#define	ACL_VAR_AIO_QUEUE_DIR		"aio_queue_dir"
#define	ACL_DEF_AIO_QUEUE_DIR		"/opt/acl_master/var/queue"
extern char *acl_var_aio_queue_dir;

#define	ACL_VAR_AIO_PID_DIR		"aio_pid_dir"
#define	ACL_DEF_AIO_PID_DIR		"/opt/acl_master/var/pid"
extern char *acl_var_aio_pid_dir;

#define	ACL_VAR_AIO_ACCESS_ALLOW	"aio_access_allow"
#define	ACL_DEF_AIO_ACCESS_ALLOW	"0.0.0.0:255.255.255.255"
extern char *acl_var_aio_access_allow;

#define	ACL_VAR_AIO_OWNER		"aio_owner"
#define	ACL_DEF_AIO_OWNER		"aio"
extern char *acl_var_aio_owner;

#define	ACL_VAR_AIO_DELAY_SEC		"aio_delay_sec"
#define	ACL_DEF_AIO_DELAY_SEC		1
extern int   acl_var_aio_delay_sec;

#define	ACL_VAR_AIO_DELAY_USEC		"aio_delay_usec"
#define	ACL_DEF_AIO_DELAY_USEC		5000
extern int   acl_var_aio_delay_usec;

#define	ACL_VAR_AIO_EVENT_MODE		"aio_event_mode"
#define	ACL_DEF_AIO_EVENT_MODE		"select"
extern char *acl_var_aio_event_mode;

#define	ACL_VAR_AIO_DAEMON_TIMEOUT	"aio_daemon_timeout"
#define	ACL_DEF_AIO_DAEMON_TIMEOUT	1800	
extern int   acl_var_aio_daemon_timeout;

#define	ACL_VAR_AIO_USE_LIMIT		"aio_use_limit"
#define	ACL_DEF_AIO_USE_LIMIT		0
extern int   acl_var_aio_use_limit;

#define	ACL_VAR_AIO_MASTER_MAXPROC	"master_maxproc"
#define	ACL_DEF_AIO_MASTER_MAXPROC	1
extern int   acl_var_aio_master_maxproc;

#define	ACL_VAR_AIO_MAX_ACCEPT		"aio_max_accept"
#define	ACL_DEF_AIO_MAX_ACCEPT		10
extern int   acl_var_aio_max_accept;

#define ACL_VAR_AIO_MIN_NOTIFY		"aio_min_notify"
#define ACL_DEF_AIO_MIN_NOTIFY		10
extern int   acl_var_aio_min_notify;

#define	ACL_VAR_AIO_ACCEPT_ALONE	"aio_accept_alone"
#define	ACL_DEF_AIO_ACCEPT_ALONE	"yes"
extern char *acl_var_aio_accept_alone;

#define	ACL_VAR_AIO_ENABLE_CORE		"aio_enable_core"
#define	ACL_DEF_AIO_ENABLE_CORE		1
extern int   acl_var_aio_enable_core;

#define ACL_VAR_AIO_DISABLE_CORE_ONEXIT	"aio_disable_core_onexit"
#define ACL_DEF_AIO_DISABLE_CORE_ONEXIT	1
extern int   acl_var_aio_disable_core_onexit;

#define ACL_VAR_AIO_CORE_LIMIT		"aio_core_limit"
#define ACL_DEF_AIO_CORE_LIMIT		-1
extern long long int acl_var_aio_core_limit;

#define	ACL_VAR_AIO_QUICK_ABORT		"aio_quick_abort"
#define	ACL_DEF_AIO_QUICK_ABORT		1
extern int   acl_var_aio_quick_abort;

#define	ACL_VAR_AIO_ACCEPT_TIMER	"aio_accept_timer"
#define	ACL_DEF_AIO_ACCEPT_TIMER	0
extern int   acl_var_aio_accept_timer;

#define	ACL_VAR_AIO_LOG_DEBUG		"master_debug"
#define	ACL_DEF_AIO_LOG_DEBUG		""
extern char *acl_var_aio_log_debug;

#define	ACL_VAR_AIO_MAX_DEBUG		"master_debug_max"
#define	ACL_DEF_AIO_MAX_DEBUG		1000
extern int   acl_var_aio_max_debug;

#define	ACL_VAR_AIO_STATUS_NOTIFY	"master_status_notify"
#define	ACL_DEF_AIO_STATUS_NOTIFY	1
extern int   acl_var_aio_status_notify;

#define	ACL_VAR_AIO_DISPATCH_ADDR	"aio_dispatch_addr"
#define	ACL_DEF_AIO_DISPATCH_ADDR	""
extern char *acl_var_aio_dispatch_addr;

#define	ACL_VAR_AIO_DISPATCH_TYPE	"aio_dispatch_type"
#define	ACL_DEF_AIO_DISPATCH_TYPE	"default"
extern char *acl_var_aio_dispatch_type;

#define	ACL_VAR_AIO_DENY_INFO		"master_deny_info"
#define	ACL_DEF_AIO_DENY_INFO		"you're not welcome!"
extern char *acl_var_aio_deny_info;

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

