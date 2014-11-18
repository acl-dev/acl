#ifndef	ACL_TRIGGER_PARAMS_INCLUDE_H
#define	ACL_TRIGGER_PARAMS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef ACL_UNIX

extern int   acl_var_trigger_pid;		/* get by call getpid() */
extern char *acl_var_trigger_procname;	/* get from trigger_main()'s argv[0] */
extern char *acl_var_trigger_log_file;	/* get from getenv("LOG") */

#define	ACL_VAR_TRIGGER_BUF_SIZE	"trigger_buf_size"
#define	ACL_DEF_TRIGGER_BUF_SIZE	81920
extern int   acl_var_trigger_buf_size;

#define	ACL_VAR_TRIGGER_RW_TIMEOUT	"trigger_rw_timeout"
#define	ACL_DEF_TRIGGER_RW_TIMEOUT	30
extern int   acl_var_trigger_rw_timeout;

#define	ACL_VAR_TRIGGER_IN_FLOW_DELAY	"trigger_in_flow_delay"
#define	ACL_DEF_TRIGGER_IN_FLOW_DELAY	1
extern int   acl_var_trigger_in_flow_delay;

#define	ACL_VAR_TRIGGER_IDLE_LIMIT	"trigger_idle_limit"
#define	ACL_DEF_TRIGGER_IDLE_LIMIT	180
extern int   acl_var_trigger_idle_limit;

#define	ACL_VAR_TRIGGER_QUEUE_DIR	"trigger_queue_dir"
#define	ACL_DEF_TRIGGER_QUEUE_DIR	"/opt/acl_master/var/queue"
extern char *acl_var_trigger_queue_dir;

#define	ACL_VAR_TRIGGER_PID_DIR		"trigger_pid_dir"
#define	ACL_DEF_TRIGGER_PID_DIR		"/opt/acl_master/var/pid"
extern char *acl_var_trigger_pid_dir;

#define	ACL_VAR_TRIGGER_OWNER		"trigger_owner"
#define	ACL_DEF_TRIGGER_OWNER		"trigger"
extern char *acl_var_trigger_owner;

#define	ACL_VAR_TRIGGER_DELAY_SEC	"trigger_delay_sec"
#define	ACL_DEF_TRIGGER_DELAY_SEC	1
extern int   acl_var_trigger_delay_sec;

#define	ACL_VAR_TRIGGER_DELAY_USEC	"trigger_delay_usec"
#define	ACL_DEF_TRIGGER_DELAY_USEC	5000
extern int   acl_var_trigger_delay_usec;

#define	ACL_VAR_TRIGGER_DAEMON_TIMEOUT	"trigger_daemon_timeout"
#define	ACL_DEF_TRIGGER_DAEMON_TIMEOUT	1800	
extern int   acl_var_trigger_daemon_timeout;

#define	ACL_VAR_TRIGGER_USE_LIMIT	"trigger_use_limit"
#define	ACL_DEF_TRIGGER_USE_LIMIT	10
extern int   acl_var_trigger_use_limit;

#define	ACL_VAR_TRIGGER_ENABLE_CORE	"trigger_enable_core"
#define	ACL_DEF_TRIGGER_ENABLE_CORE	1
extern int   acl_var_trigger_enable_core;

#define	ACL_VAR_TRIGGER_LOG_DEBUG	"master_debug"
#define	ACL_DEF_TRIGGER_LOG_DEBUG	""
extern char *acl_var_trigger_log_debug;

#define	ACL_VAR_TRIGGER_MAX_DEBUG	"master_debug_max"
#define	ACL_DEF_TRIGGER_MAX_DEBUG	1000
extern int   acl_var_trigger_max_debug;

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif

