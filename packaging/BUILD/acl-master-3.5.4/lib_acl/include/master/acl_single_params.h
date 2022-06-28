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

#define	ACL_VAR_SINGLE_BUF_SIZE			"single_buf_size"
#define	ACL_DEF_SINGLE_BUF_SIZE			81920
extern int   acl_var_single_buf_size;

#define	ACL_VAR_SINGLE_RW_TIMEOUT		"single_rw_timeout"
#define	ACL_DEF_SINGLE_RW_TIMEOUT		30
extern int   acl_var_single_rw_timeout;

#define	ACL_VAR_SINGLE_IN_FLOW_DELAY		"single_in_flow_delay"
#define	ACL_DEF_SINGLE_IN_FLOW_DELAY		1
extern int   acl_var_single_in_flow_delay;

/*
 * Any subsystem: default amount of time a mail subsystem waits for a client
 * connection (except queue manager).
 */
#define	ACL_VAR_SINGLE_IDLE_LIMIT		"single_idle_limit"
#define	ACL_DEF_SINGLE_IDLE_LIMIT		0
extern int   acl_var_single_idle_limit;

#define	ACL_VAR_SINGLE_QUEUE_DIR		"single_queue_dir"
#define	ACL_DEF_SINGLE_QUEUE_DIR		"/opt/acl_master/var/queue"
extern char *acl_var_single_queue_dir;

#define	ACL_VAR_SINGLE_PID_DIR			"single_pid_dir"
#define	ACL_DEF_SINGLE_PID_DIR			"/opt/acl_master/var/pid"
extern char *acl_var_single_pid_dir;

#define	ACL_VAR_SINGLE_OWNER			"single_owner"
#define	ACL_DEF_SINGLE_OWNER			"root"
extern char *acl_var_single_owner;

#define	ACL_VAR_SINGLE_DELAY_SEC		"single_delay_sec"
#define	ACL_DEF_SINGLE_DELAY_SEC		1
extern int   acl_var_single_delay_sec;

#define	ACL_VAR_SINGLE_DELAY_USEC		"single_delay_usec"
#define	ACL_DEF_SINGLE_DELAY_USEC		5000
extern int   acl_var_single_delay_usec;

/*
 * How long a daemon command may take to receive or deliver a message etc.
 * before we assume it is wegded (should never happen).
 */
#define	ACL_VAR_SINGLE_DAEMON_TIMEOUT		"single_daemon_timeout"
#define	ACL_DEF_SINGLE_DAEMON_TIMEOUT		1800	
extern int   acl_var_single_daemon_timeout;

/*
 * Any subsystem: default maximum number of clients serviced before a mail
 * subsystem terminates (except queue manager).
 */
#define	ACL_VAR_SINGLE_USE_LIMIT		"single_use_limit"
#define	ACL_DEF_SINGLE_USE_LIMIT		0
extern int   acl_var_single_use_limit;

#define	ACL_VAR_SINGLE_ENABLE_CORE		"single_enable_core"
#define	ACL_DEF_SINGLE_ENABLE_CORE		1
extern int   acl_var_single_enable_core;

#define	ACL_VAR_SINGLE_DISABLE_CORE_ONEXIT	"single_disable_core_onexit"
#define	ACL_DEF_SINGLE_DISABLE_CORE_ONEXIT	1
extern int   acl_var_single_disable_core_onexit;

#define ACL_VAR_SINGLE_CORE_LIMIT		"single_core_limit"
#define	ACL_DEF_SINGLE_CORE_LIMIT		-1
extern long long int acl_var_single_core_limit;

#define	ACL_VAR_SINGLE_LOG_DEBUG		"master_debug"
#define	ACL_DEF_SINGLE_LOG_DEBUG		""
extern char *acl_var_single_log_debug;

#define	ACL_VAR_SINGLE_MAX_DEBUG		"master_debug_max"
#define	ACL_DEF_SINGLE_MAX_DEBUG		1000
extern int   acl_var_single_max_debug;

#endif /* ACL_UNIX*/

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif

