#ifndef	ACL_THREADS_PARAMS_INCLUDE_H
#define	ACL_THREADS_PARAMS_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

extern char *acl_var_threads_procname;
extern char *acl_var_threads_log_file;

#define	ACL_VAR_THREADS_BUF_SIZE		"ioctl_buf_size"
#define	ACL_DEF_THREADS_BUF_SIZE		8192
extern int   acl_var_threads_buf_size;

#define	ACL_VAR_THREADS_RW_TIMEOUT		"ioctl_rw_timeout"
#define	ACL_DEF_THREADS_RW_TIMEOUT		30
extern int   acl_var_threads_rw_timeout;

#define	ACL_VAR_THREADS_POOL_LIMIT		"ioctl_max_threads"
#define	ACL_DEF_THREADS_POOL_LIMIT		50
extern int   acl_var_threads_pool_limit;

#define	ACL_VAR_THREADS_THREAD_STACKSIZE	"ioctl_stacksize"
#define	ACL_DEF_THREADS_THREAD_STACKSIZE	0
extern int   acl_var_threads_thread_stacksize;

#define	ACL_VAR_THREADS_THREAD_IDLE		"ioctl_thread_idle_limit"
#define	ACL_DEF_THREADS_THREAD_IDLE		120
extern int   acl_var_threads_thread_idle;

#define	ACL_VAR_THREADS_IDLE_LIMIT		"ioctl_idle_limit"
#define	ACL_DEF_THREADS_IDLE_LIMIT		0
extern int   acl_var_threads_idle_limit;

#define	ACL_VAR_THREADS_USE_LIMIT		"ioctl_use_limit"
#define	ACL_DEF_THREADS_USE_LIMIT		0
extern int   acl_var_threads_use_limit;

#define	ACL_VAR_THREADS_QUEUE_DIR		"ioctl_queue_dir"
#define	ACL_DEF_THREADS_QUEUE_DIR		"/opt/acl_master/var/queue"
extern char *acl_var_threads_queue_dir;

#define	ACL_VAR_THREADS_OWNER			"ioctl_owner"
#define	ACL_DEF_THREADS_OWNER			"root"
extern char *acl_var_threads_owner;

#define	ACL_VAR_THREADS_DELAY_SEC		"ioctl_delay_sec"
#define	ACL_DEF_THREADS_DELAY_SEC		1
extern int   acl_var_threads_delay_sec;

#define	ACL_VAR_THREADS_DELAY_USEC		"ioctl_delay_usec"
#define	ACL_DEF_THREADS_DELAY_USEC		5000
extern int   acl_var_threads_delay_usec;

#define	ACL_VAR_THREADS_EVENT_MODE		"ioctl_event_mode"
#ifdef ACL_UNIX
#define	ACL_DEF_THREADS_EVENT_MODE		"kernel"
#else
#define	ACL_DEF_THREADS_EVENT_MODE		"select"
#endif
extern char *acl_var_threads_event_mode;

#define	ACL_VAR_THREADS_DAEMON_TIMEOUT		"ioctl_daemon_timeout"
#define	ACL_DEF_THREADS_DAEMON_TIMEOUT		1800	
extern int   acl_var_threads_daemon_timeout;

#define	ACL_VAR_THREADS_MASTER_MAXPROC		"master_maxproc"
#define	ACL_DEF_THREADS_MASTER_MAXPROC		1
extern int   acl_var_threads_master_maxproc;

#define	ACL_VAR_THREADS_MAX_ACCEPT		"ioctl_max_accept"
#define	ACL_DEF_THREADS_MAX_ACCEPT		15
extern int   acl_var_threads_max_accept;

#define	ACL_VAR_THREADS_ENABLE_DOG		"ioctl_enable_dog"
#ifdef ACL_UNIX
#define	ACL_DEF_THREADS_ENABLE_DOG		0
#else
#define	ACL_DEF_THREADS_ENABLE_DOG		1
#endif
extern int   acl_var_threads_enable_dog;

#define	ACL_VAR_THREADS_QUICK_ABORT		"ioctl_quick_abort"
#define	ACL_DEF_THREADS_QUICK_ABORT		1
extern int   acl_var_threads_quick_abort;

#define	ACL_VAR_THREADS_ENABLE_CORE		"ioctl_enable_core"
#define	ACL_DEF_THREADS_ENABLE_CORE		1
extern int   acl_var_threads_enable_core;

#define	ACL_VAR_THREADS_DISABLE_CORE_ONEXIT	"ioctl_disable_core_onexit"
#define	ACL_DEF_THREADS_DISABLE_CORE_ONEXIT	1
extern int   acl_var_threads_disable_core_onexit;

#define ACL_VAR_THREADS_CORE_LIMIT		"ioctl_core_limit"
#define ACL_DEF_THREADS_CORE_LIMIT		-1
extern long long int acl_var_threads_core_limit;

#define	ACL_VAR_THREADS_LOG_DEBUG		"master_debug"
#define	ACL_DEF_THREADS_LOG_DEBUG		""
extern char *acl_var_threads_log_debug;

#define	ACL_VAR_THREADS_MAX_DEBUG		"master_debug_max"
#define	ACL_DEF_THREADS_MAX_DEBUG		1000
extern int   acl_var_threads_max_debug;

#define	ACL_VAR_THREADS_STATUS_NOTIFY		"master_status_notify"
#define	ACL_DEF_THREADS_STATUS_NOTIFY		1
extern int   acl_var_threads_status_notify;

#define	ACL_VAR_THREADS_DENY_BANNER		"ioctl_deny_banner"
#define	ACL_DEF_THREADS_DENY_BANNER		"You'are not Welcome!"
extern char *acl_var_threads_deny_banner;

#define	ACL_VAR_THREADS_ACCESS_ALLOW		"ioctl_access_allow"
#define	ACL_DEF_THREADS_ACCESS_ALLOW		"all"
extern char *acl_var_threads_access_allow;

#define	ACL_VAR_THREADS_BATADD			"ioctl_batadd"
#define	ACL_DEF_THREADS_BATADD			0
extern int   acl_var_threads_batadd;

#define	ACL_VAR_THREADS_SCHEDULE_WARN		"ioctl_schedule_warn"
#define	ACL_DEF_THREADS_SCHEDULE_WARN		100
extern int   acl_var_threads_schedule_warn;

#define	ACL_VAR_THREADS_SCHEDULE_WAIT		"ioctl_schedule_wait"
#define	ACL_DEF_THREADS_SCHEDULE_WAIT		50
extern int   acl_var_threads_schedule_wait;

#define	ACL_VAR_THREADS_CHECK_INTER		"ioctl_check_inter"
#define	ACL_DEF_THREADS_CHECK_INTER		100
extern int   acl_var_threads_check_inter;

#define	ACL_VAR_THREADS_QLEN_WARN		"ioctl_qlen_warn"
#define	ACL_DEF_THREADS_QLEN_WARN		0
extern int   acl_var_threads_qlen_warn;

#define	ACL_VAR_THREADS_DISPATCH_ADDR		"ioctl_dispatch_addr"
#define	ACL_DEF_THREADS_DISPATCH_ADDR		""
extern char *acl_var_threads_dispatch_addr;

#define	ACL_VAR_THREADS_DISPATCH_TYPE		"ioctl_dispatch_type"
#define	ACL_DEF_THREADS_DISPATCH_TYPE		"default"
extern char *acl_var_threads_dispatch_type;

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
