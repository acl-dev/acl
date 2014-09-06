#ifndef	ACL_IOCTL_PARAMS_INCLUDE_H
#define	ACL_IOCTL_PARAMS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef ACL_UNIX

extern int   acl_var_ioctl_pid;
extern char *acl_var_ioctl_procname;
extern char *acl_var_ioctl_log_file;

#define	ACL_VAR_IOCTL_BUF_SIZE		"ioctl_buf_size"
#define	ACL_DEF_IOCTL_BUF_SIZE		81920
extern int   acl_var_ioctl_buf_size;

#define	ACL_VAR_IOCTL_RW_TIMEOUT	"ioctl_rw_timeout"
#define	ACL_DEF_IOCTL_RW_TIMEOUT	30
extern int   acl_var_ioctl_rw_timeout;

#define	ACL_VAR_IOCTL_IN_FLOW_DELAY	"ioctl_in_flow_delay"
#define	ACL_DEF_IOCTL_IN_FLOW_DELAY	1
extern int   acl_var_ioctl_in_flow_delay;

#define	ACL_VAR_IOCTL_MAX_THREADS	"ioctl_max_threads"
#define	ACL_DEF_IOCTL_MAX_THREADS	50
extern int   acl_var_ioctl_max_threads;

#define	ACL_VAR_IOCTL_STACKSIZE	"ioctl_stacksize"
#define	ACL_DEF_IOCTL_STACKSIZE		0
extern int   acl_var_ioctl_stacksize;

#define	ACL_VAR_IOCTL_THREAD_IDLE_LIMIT	"ioctl_thread_idle_limit"
#define	ACL_DEF_IOCTL_THREAD_IDLE_LIMIT	180
extern int   acl_var_ioctl_thread_idle_limit;

#define	ACL_VAR_IOCTL_IDLE_LIMIT	"ioctl_idle_limit"
#define	ACL_DEF_IOCTL_IDLE_LIMIT	180
extern int   acl_var_ioctl_idle_limit;

#define	ACL_VAR_IOCTL_QUEUE_DIR		"ioctl_queue_dir"
#define	ACL_DEF_IOCTL_QUEUE_DIR		"/opt/acl_master/var/queue"
extern char *acl_var_ioctl_queue_dir;

#define	ACL_VAR_IOCTL_PID_DIR		"ioctl_pid_dir"
#define	ACL_DEF_IOCTL_PID_DIR		"/opt/acl_master/var/pid"
extern char *acl_var_ioctl_pid_dir;

#define	ACL_VAR_IOCTL_ACCESS_ALLOW	"ioctl_access_allow"
#define	ACL_DEF_IOCTL_ACCESS_ALLOW	"0.0.0.0:255.255.255.255"
extern char *acl_var_ioctl_access_allow;

#define	ACL_VAR_IOCTL_OWNER		"ioctl_owner"
#define	ACL_DEF_IOCTL_OWNER		"ioctl"
extern char *acl_var_ioctl_owner;

#define	ACL_VAR_IOCTL_DELAY_SEC		"ioctl_delay_sec"
#define	ACL_DEF_IOCTL_DELAY_SEC		1
extern int   acl_var_ioctl_delay_sec;

#define	ACL_VAR_IOCTL_DELAY_USEC	"ioctl_delay_usec"
#define	ACL_DEF_IOCTL_DELAY_USEC	5000
extern int   acl_var_ioctl_delay_usec;

#define	ACL_VAR_IOCTL_EVENT_MODE	"ioctl_event_mode"
#define	ACL_DEF_IOCTL_EVENT_MODE	"select"
extern char *acl_var_ioctl_event_mode;

#define	ACL_VAR_IOCTL_DAEMON_TIMEOUT	"ioctl_daemon_timeout"
#define	ACL_DEF_IOCTL_DAEMON_TIMEOUT	1800	
extern int   acl_var_ioctl_daemon_timeout;

#define	ACL_VAR_IOCTL_USE_LIMIT		"ioctl_use_limit"
#define	ACL_DEF_IOCTL_USE_LIMIT		10
extern int   acl_var_ioctl_use_limit;

#define	ACL_VAR_IOCTL_MASTER_MAXPROC	"master_maxproc"
#define	ACL_DEF_IOCTL_MASTER_MAXPROC	1
extern int   acl_var_ioctl_master_maxproc;

#define	ACL_VAR_IOCTL_MAX_ACCEPT	"ioctl_max_accept"
#define	ACL_DEF_IOCTL_MAX_ACCEPT	15
extern int   acl_var_ioctl_max_accept;

#define	ACL_VAR_IOCTL_ENABLE_DOG	"ioctl_enable_dog"
#define	ACL_DEF_IOCTL_ENABLE_DOG	1
extern int   acl_var_ioctl_enable_dog;

#define	ACL_VAR_IOCTL_QUICK_ABORT	"ioctl_quick_abort"
#define	ACL_DEF_IOCTL_QUICK_ABORT	1
extern int   acl_var_ioctl_quick_abort;

#define	ACL_VAR_IOCTL_ENABLE_CORE	"ioctl_enable_core"
#define	ACL_DEF_IOCTL_ENABLE_CORE	1
extern int   acl_var_ioctl_enable_core;

#define	ACL_VAR_IOCTL_LOG_DEBUG		"master_debug"
#define	ACL_DEF_IOCTL_LOG_DEBUG		""
extern char *acl_var_ioctl_log_debug;

#define	ACL_VAR_IOCTL_MAX_DEBUG		"master_debug_max"
#define	ACL_DEF_IOCTL_MAX_DEBUG		1000
extern int   acl_var_ioctl_max_debug;

#define	ACL_VAR_IOCTL_STATUS_NOTIFY	"master_status_notify"
#define	ACL_DEF_IOCTL_STATUS_NOTIFY	1
extern int   acl_var_ioctl_status_notify;

#define	ACL_VAR_IOCTL_CHECK_INTER	"ioctl_check_inter"
#define	ACL_DEF_IOCTL_CHECK_INTER	100
extern int   acl_var_ioctl_check_inter;

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif

