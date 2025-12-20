#ifndef	ACL_THREADS_PARAMS_INCLUDE_H
#define	ACL_THREADS_PARAMS_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

extern char *acl_var_threads_procname;
extern char *acl_var_threads_log_file;

extern int   acl_var_threads_buf_size;
extern int   acl_var_threads_rw_timeout;
extern int   acl_var_threads_pool_limit;
extern int   acl_var_threads_thread_stacksize;
extern int   acl_var_threads_thread_idle;
extern int   acl_var_threads_idle_limit;
extern int   acl_var_threads_use_limit;

extern char *acl_var_threads_queue_dir;
extern char *acl_var_threads_owner;
extern int   acl_var_threads_delay_sec;
extern int   acl_var_threads_delay_usec;
extern char *acl_var_threads_event_mode;
extern int   acl_var_threads_daemon_timeout;
extern int   acl_var_threads_master_maxproc;
extern int   acl_var_threads_max_accept;
extern int   acl_var_threads_enable_dog;
extern int   acl_var_threads_quick_abort;
extern int   acl_var_threads_enable_core;
extern int   acl_var_threads_disable_core_onexit;
extern long long int acl_var_threads_core_limit;
extern char *acl_var_threads_log_debug;
extern int   acl_var_threads_max_debug;
extern int   acl_var_threads_status_notify;
extern char *acl_var_threads_deny_banner;
extern char *acl_var_threads_access_allow;
extern int   acl_var_threads_batadd;
extern int   acl_var_threads_schedule_warn;
extern int   acl_var_threads_schedule_wait;
extern int   acl_var_threads_check_inter;
extern int   acl_var_threads_qlen_warn;
extern char *acl_var_threads_dispatch_addr;
extern char *acl_var_threads_dispatch_type;
extern char *acl_var_threads_master_service;
extern char *acl_var_threads_master_reuseport;

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
