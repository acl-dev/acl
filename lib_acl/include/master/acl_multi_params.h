#ifndef	ACL_MULTI_PARAMS_INCLUDE_H
#define	ACL_MULTI_PARAMS_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef ACL_UNIX

extern int   acl_var_multi_pid;
extern char *acl_var_multi_procname;
extern char *acl_var_multi_log_file;

#define	ACL_VAR_MULTI_BUF_SIZE		"multi_buf_size"
#define	ACL_DEF_MULTI_BUF_SIZE		81920
extern int   acl_var_multi_buf_size;

#define	ACL_VAR_MULTI_RW_TIMEOUT	"multi_rw_timeout"
#define	ACL_DEF_MULTI_RW_TIMEOUT	30
extern int   acl_var_multi_rw_timeout;

#define	ACL_VAR_MULTI_IN_FLOW_DELAY	"multi_in_flow_delay"
#define	ACL_DEF_MULTI_IN_FLOW_DELAY	1
extern int   acl_var_multi_in_flow_delay;

#define	ACL_VAR_MULTI_IDLE_LIMIT	"multi_idle_limit"
#define	ACL_DEF_MULTI_IDLE_LIMIT	180
extern int   acl_var_multi_idle_limit;

#define	ACL_VAR_MULTI_QUEUE_DIR		"multi_queue_dir"
#define	ACL_DEF_MULTI_QUEUE_DIR		"/opt/acl_master/var/queue"
extern char *acl_var_multi_queue_dir;

#define	ACL_VAR_MULTI_PID_DIR		"multi_pid_dir"
#define	ACL_DEF_MULTI_PID_DIR		"/opt/acl_master/var/pid"
extern char *acl_var_multi_pid_dir;

#define	ACL_VAR_MULTI_OWNER		"multi_owner"
#define	ACL_DEF_MULTI_OWNER		"multi"
extern char *acl_var_multi_owner;

#define	ACL_VAR_MULTI_DELAY_SEC		"multi_delay_sec"
#define	ACL_DEF_MULTI_DELAY_SEC		1
extern int   acl_var_multi_delay_sec;

#define	ACL_VAR_MULTI_DELAY_USEC	"multi_delay_usec"
#define	ACL_DEF_MULTI_DELAY_USEC	5000
extern int   acl_var_multi_delay_usec;

#define	ACL_VAR_MULTI_DAEMON_TIMEOUT	"multi_daemon_timeout"
#define	ACL_DEF_MULTI_DAEMON_TIMEOUT	1800	
extern int   acl_var_multi_daemon_timeout;

#define	ACL_VAR_MULTI_USE_LIMIT		"multi_use_limit"
#define	ACL_DEF_MULTI_USE_LIMIT		10
extern int   acl_var_multi_use_limit;

#define	ACL_VAR_MULTI_ENABLE_CORE	"multi_enable_core"
#define	ACL_DEF_MULTI_ENABLE_CORE	1
extern int   acl_var_multi_enable_core;

#define	ACL_VAR_MULTI_LOG_DEBUG		"master_debug"
#define	ACL_DEF_MULTI_LOG_DEBUG		""
extern char *acl_var_multi_log_debug;

#define	ACL_VAR_MULTI_MAX_DEBUG		"master_debug_max"
#define	ACL_DEF_MULTI_MAX_DEBUG		1000
extern int   acl_var_multi_max_debug;

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif

