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

extern int   acl_var_aio_buf_size;
extern int   acl_var_aio_rw_timeout;
extern int   acl_var_aio_in_flow_delay;
extern int   acl_var_aio_max_threads;
extern int   acl_var_aio_thread_idle_limit;
extern int   acl_var_aio_idle_limit;
extern char *acl_var_aio_queue_dir;
extern char *acl_var_aio_pid_dir;
extern char *acl_var_aio_access_allow;
extern char *acl_var_aio_owner;
extern int   acl_var_aio_delay_sec;
extern int   acl_var_aio_delay_usec;
extern char *acl_var_aio_event_mode;
extern int   acl_var_aio_daemon_timeout;
extern int   acl_var_aio_use_limit;
extern int   acl_var_aio_master_maxproc;
extern int   acl_var_aio_max_accept;
extern int   acl_var_aio_min_notify;
extern char *acl_var_aio_accept_alone;
extern int   acl_var_aio_enable_core;
extern int   acl_var_aio_disable_core_onexit;
extern long long int acl_var_aio_core_limit;
extern int   acl_var_aio_quick_abort;
extern int   acl_var_aio_accept_timer;
extern char *acl_var_aio_log_debug;
extern int   acl_var_aio_max_debug;
extern int   acl_var_aio_status_notify;
extern char *acl_var_aio_dispatch_addr;
extern char *acl_var_aio_dispatch_type;
extern char *acl_var_aio_deny_info;

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

