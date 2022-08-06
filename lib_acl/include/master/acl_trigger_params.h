#ifndef	ACL_TRIGGER_PARAMS_INCLUDE_H
#define	ACL_TRIGGER_PARAMS_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef ACL_UNIX

extern int   acl_var_trigger_pid;	/* get by call getpid() */
extern char *acl_var_trigger_procname;	/* get from trigger_main()'s argv[0] */
extern char *acl_var_trigger_log_file;	/* get from getenv("LOG") */

extern int   acl_var_trigger_buf_size;
extern int   acl_var_trigger_rw_timeout;
extern int   acl_var_trigger_in_flow_delay;
extern int   acl_var_trigger_idle_limit;
extern char *acl_var_trigger_queue_dir;
extern char *acl_var_trigger_pid_dir;
extern char *acl_var_trigger_owner;
extern int   acl_var_trigger_delay_sec;
extern int   acl_var_trigger_delay_usec;
extern int   acl_var_trigger_daemon_timeout;
extern int   acl_var_trigger_use_limit;
extern int   acl_var_trigger_enable_core;
extern int   acl_var_trigger_disable_core_onexit;
extern long long int acl_var_trigger_core_limit;
extern char *acl_var_trigger_log_debug;
extern int   acl_var_trigger_max_debug;

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

