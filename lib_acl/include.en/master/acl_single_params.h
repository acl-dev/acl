#ifndef	ACL_SINGLE_PARAMS_INCLUDE_H
#define	ACL_SINGLE_PARAMS_INCLUDE_H

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef ACL_UNIX

extern int   acl_var_single_pid;	/* get by call getpid() */
extern char *acl_var_single_procname;	/* get from single_main()'s argv[0] */
extern char *acl_var_single_log_file;	/* get from getenv("LOG") */

extern int   acl_var_single_buf_size;
extern int   acl_var_single_rw_timeout;
extern int   acl_var_single_in_flow_delay;

/*
 * Any subsystem: default amount of time a mail subsystem waits for a client
 * connection (except queue manager).
 */
extern int   acl_var_single_idle_limit;
extern char *acl_var_single_queue_dir;
extern char *acl_var_single_pid_dir;
extern char *acl_var_single_owner;
extern int   acl_var_single_delay_sec;
extern int   acl_var_single_delay_usec;

/*
 * How long a daemon command may take to receive or deliver a message etc.
 * before we assume it is wegded (should never happen).
 */
extern int   acl_var_single_daemon_timeout;

/*
 * Any subsystem: default maximum number of clients serviced before a mail
 * subsystem terminates (except queue manager).
 */
extern int   acl_var_single_use_limit;
extern int   acl_var_single_enable_core;
extern int   acl_var_single_disable_core_onexit;
extern long long int acl_var_single_core_limit;
extern char *acl_var_single_log_debug;
extern int   acl_var_single_max_debug;

#endif /* ACL_UNIX*/

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

