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

#define	ACL_VAR_UDP_BUF_SIZE			"udp_bufsize"
#define	ACL_DEF_UDP_BUF_SIZE			4096
extern int   acl_var_udp_buf_size;

#define	ACL_VAR_UDP_RW_TIMEOUT			"udp_rw_timeout"
#define	ACL_DEF_UDP_RW_TIMEOUT			30
extern int   acl_var_udp_rw_timeout;

#define ACL_VAR_UDP_USE_LIMIT			"udp_use_limit"
#define ACL_DEF_UDP_USE_LIMIT			0
extern long long int acl_var_udp_use_limit;

#define	ACL_VAR_UDP_IDLE_LIMIT			"udp_idle_limit"
#define	ACL_DEF_UDP_IDLE_LIMIT			0
extern int   acl_var_udp_idle_limit;

#define	ACL_VAR_UDP_QUEUE_DIR			"udp_queue_dir"
#define	ACL_DEF_UDP_QUEUE_DIR			"/opt/acl_master/var/queue"
extern char *acl_var_udp_queue_dir;

#define	ACL_VAR_UDP_PID_DIR			"udp_pid_dir"
#define	ACL_DEF_UDP_PID_DIR			"/opt/acl_master/var/pid"
extern char *acl_var_udp_pid_dir;

#define	ACL_VAR_UDP_ACCESS_ALLOW		"udp_access_allow"
#define	ACL_DEF_UDP_ACCESS_ALLOW		"0.0.0.0:255.255.255.255"
extern char *acl_var_udp_access_allow;

#define	ACL_VAR_UDP_OWNER			"udp_owner"
#define	ACL_DEF_UDP_OWNER			"root"
extern char *acl_var_udp_owner;

#define	ACL_VAR_UDP_DELAY_SEC			"udp_delay_sec"
#define	ACL_DEF_UDP_DELAY_SEC			1
extern int   acl_var_udp_delay_sec;

#define	ACL_VAR_UDP_DELAY_USEC			"udp_delay_usec"
#define	ACL_DEF_UDP_DELAY_USEC			5000
extern int   acl_var_udp_delay_usec;

#define	ACL_VAR_UDP_EVENT_MODE			"udp_event_mode"
#define	ACL_DEF_UDP_EVENT_MODE			"select"
extern char *acl_var_udp_event_mode;

#define	ACL_VAR_UDP_DAEMON_TIMEOUT		"udp_daemon_timeout"
#define	ACL_DEF_UDP_DAEMON_TIMEOUT		1800	
extern int   acl_var_udp_daemon_timeout;

#define	ACL_VAR_UDP_MASTER_MAXPROC		"master_maxproc"
#define	ACL_DEF_UDP_MASTER_MAXPROC		1
extern int   acl_var_udp_master_maxproc;

#define	ACL_VAR_UDP_ENABLE_CORE			"udp_enable_core"
#define	ACL_DEF_UDP_ENABLE_CORE			1
extern int   acl_var_udp_enable_core;

#define	ACL_VAR_UDP_DISABLE_CORE_ONEXIT		"udp_disable_core_onexit"
#define	ACL_DEF_UDP_DISABLE_CORE_ONEXIT		1
extern int   acl_var_udp_disable_core_onexit;

#define ACL_VAR_UDP_CORE_LIMIT			"udp_core_limit"
#define ACL_DEF_UDP_CORE_LIMIT			-1
extern long long int acl_var_udp_core_limit;

#define	ACL_VAR_UDP_LOG_DEBUG			"master_debug"
#define	ACL_DEF_UDP_LOG_DEBUG			""
extern char *acl_var_udp_log_debug;

#define	ACL_VAR_UDP_MAX_DEBUG			"master_debug_max"
#define	ACL_DEF_UDP_MAX_DEBUG			1000
extern int   acl_var_udp_max_debug;

#define ACL_VAR_UDP_THREADS			"udp_threads"
#define ACL_DEF_UDP_THREADS			1
extern int   acl_var_udp_threads;

#define ACL_VAR_UDP_THREADS_DETACHED		"udp_threads_detached"
#define ACL_DEF_UDP_THREADS_DETACHED		1
extern int   acl_var_udp_threads_detached;

#define ACL_VAR_UDP_FATAL_ON_BIND_ERROR		"udp_fatal_on_bind_error"
#define ACL_DEF_UDP_FATAL_ON_BIND_ERROR		0
extern int   acl_var_udp_fatal_on_bind_error;

#define ACL_VAR_UDP_NON_BLOCK			"master_nonblock"
#define ACL_DEF_UDP_NON_BLOCK			1
extern int   acl_var_udp_non_block;

#define ACL_VAR_UDP_REUSEPORT			"master_reuseport"
#define ACL_DEF_UDP_REUSEPORT			"yes"
extern char *acl_var_udp_reuse_port;

#define ACL_VAR_UDP_PRIVATE			"master_private"
#define ACL_DEF_UDP_PRIVATE			"n"
extern char *acl_var_udp_private;

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
