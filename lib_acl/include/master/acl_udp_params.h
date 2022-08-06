#ifndef	__ACL_UDP_PARAMS_INCLUDE_H_
#define	__ACL_UDP_PARAMS_INCLUDE_H_

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

extern int   acl_var_udp_pid;
extern char *acl_var_udp_procname;
extern char *acl_var_udp_log_file;

extern int   acl_var_udp_buf_size;
extern int   acl_var_udp_rw_timeout;
extern long long int acl_var_udp_use_limit;
extern int   acl_var_udp_idle_limit;
extern char *acl_var_udp_queue_dir;
extern char *acl_var_udp_pid_dir;
extern char *acl_var_udp_access_allow;
extern char *acl_var_udp_owner;
extern int   acl_var_udp_delay_sec;
extern int   acl_var_udp_delay_usec;
extern char *acl_var_udp_event_mode;
extern int   acl_var_udp_daemon_timeout;
extern int   acl_var_udp_master_maxproc;
extern int   acl_var_udp_enable_core;
extern int   acl_var_udp_disable_core_onexit;
extern long long int acl_var_udp_core_limit;
extern char *acl_var_udp_log_debug;
extern int   acl_var_udp_max_debug;
extern int   acl_var_udp_threads;
extern int   acl_var_udp_threads_detached;
extern int   acl_var_udp_fatal_on_bind_error;
extern int   acl_var_udp_monitor_netlink;
extern int   acl_var_udp_non_block;
extern char *acl_var_udp_reuse_port;
extern char *acl_var_udp_private;

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
