#ifndef	__MASTER_PARAMS_INCLUDE_H__
#define	__MASTER_PARAMS_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

/* be set value in master.c */

extern char *acl_var_master_conf_dir;

 /*
  * master's main configure file
  * Virtual host support. Default is to listen on all machine interfaces.
  */
#define ACL_VAR_MASTER_INET_INTERFACES		"inet_interfaces"
#define ACL_INET_INTERFACES_ALL			"all"
#define ACL_INET_INTERFACES_LOCAL		"loopback-only"
#define ACL_DEF_MASTER_INET_INTERFACES	ACL_INET_INTERFACES_ALL
extern char *acl_var_master_inet_interfaces;

#define ACL_VAR_MASTER_STOP_KILL		"stop_kill"
#define ACL_DEF_MASTER_STOP_KILL		"true"
extern char *acl_var_master_stop_kill;

#define ACL_VAR_MASTER_PROC_LIMIT		"default_process_limit"
#define ACL_DEF_MASTER_PROC_LIMIT		100
extern int acl_var_master_proc_limit;    

extern uid_t acl_var_master_owner_uid;		/* zsx test */

#define	ACL_VAR_MASTER_OWNER_GROUP		"owner_group"
#define	ACL_DEF_MASTER_OWNER_GROUP		"master"
extern char *acl_var_master_owner_group;
extern gid_t acl_var_master_owner_gid;		/* zsx test */

#define	ACL_VAR_MASTER_THROTTLE_TIME		"service_throttle_time"
#define ACL_DEF_MASTER_THROTTLE_TIME		60
extern int   acl_var_master_throttle_time; 

#define ACL_VAR_MASTER_DAEMON_DIR		"daemon_directory"
#define ACL_DEF_MASTER_DAEMON_DIR		"/opt/acl/libexec"
extern char *acl_var_master_daemon_dir;

#define	ACL_VAR_MASTER_SERVICE_DIR		"service_directory"
#define	ACL_DEF_MASTER_SERVICE_DIR		"/opt/acl/conf/service"
extern char *acl_var_master_service_dir;

#define ACL_VAR_MASTER_FILE_EXTS		"file_exts"
#define ACL_DEF_MASTER_FILE_EXTS		"cf, conf"
extern char *acl_var_master_file_exts;

#define ACL_VAR_MASTER_SERVICE_FILE		"service_file"
#define ACL_DEF_MASTER_SERVICE_FILE		"/opt/acl/conf/services.cf"
extern char *acl_var_master_service_file;

#define	ACL_VAR_MASTER_QUEUE_DIR		"queue_directory"
#define	ACL_DEF_MASTER_QUEUE_DIR		"/opt/acl/var"
extern char *acl_var_master_queue_dir;

#define	ACL_VAR_MASTER_LOG_FILE			"log_file"
#define	ACL_DEF_MASTER_LOG_FILE			"/opt/acl/var/log/master.log"
extern char *acl_var_master_log_file;

#define	ACL_VAR_MASTER_PID_FILE			"pid_file"
#define	ACL_DEF_MASTER_PID_FILE			"/opt/acl/var/pid/master.pid"
extern char *acl_var_master_pid_file;

#define	ACL_VAR_MASTER_BUF_SIZE			"buf_size"
#define	ACL_DEF_MASTER_BUF_SIZE			81920
extern int   acl_var_master_buf_size;

#define	ACL_VAR_MASTER_RW_TIMEOUT		"rw_timeout"
#define	ACL_DEF_MASTER_RW_TIMEOUT		30
extern int   acl_var_master_rw_timeout;

#define	ACL_VAR_MASTER_SCAN_SUBDIR		"scan_subdir"
#define	ACL_DEF_MASTER_SCAN_SUBDIR		0
extern int   acl_var_master_scan_subdir;

#define	ACL_VAR_MASTER_LIMIT_PRIVILEGE		"limit_privilege"
#define	ACL_DEF_MASTER_LIMIT_PRIVILEGE		0
extern int   acl_var_master_limit_privilege;

extern pid_t acl_var_master_pid;

/**
 * Inbound mail flow control. This allows for a stiffer coupling between
 * receiving mail and sending mail. A sending process produces one token for
 * each message that it takes from the incoming queue; a receiving process
 * consumes one token for each message that it adds to the incoming queue.
 * When no token is available (Postfix receives more mail than it is able to
 * deliver) a receiving process pauses for $in_flow_delay seconds so that
 * the sending processes get a chance to access the disk.
 */
#define ACL_VAR_MASTER_IN_FLOW_DELAY		"in_flow_delay"
#ifdef ACL_PIPES_CANT_FIONREAD
#define ACL_DEF_MASTER_IN_FLOW_DELAY		0
#else
#define ACL_DEF_MASTER_IN_FLOW_DELAY		1
#endif
extern int   acl_var_master_in_flow_delay;

#define	ACL_VAR_MASTER_DELAY_SEC		"event_delay_sec"
#define	ACL_DEF_MASTER_DELAY_SEC		1
extern int   acl_var_master_delay_sec;

#define	ACL_VAR_MASTER_DELAY_USEC		"event_delay_usec"
#define	ACL_DEF_MASTER_DELAY_USEC		5000
extern int   acl_var_master_delay_usec;

#define ACL_VAR_MASTER_MANAGE_ADDR		"manage_addr"
#define ACL_DEF_MASTER_MANAGE_ADDR		""
extern char *acl_var_master_manage_addr;

#define ACL_VAR_MASTER_RELOAD_TIMEO		"reload_timeout"
#define ACL_DEF_MASTER_RELOAD_TIMEO		5000
extern int   acl_var_master_reload_timeo;

#define ACL_VAR_MASTER_START_TIMEO		"start_timeout"
#define ACL_DEF_MASTER_START_TIMEO		5000
extern int   acl_var_master_start_timeo;

/* every service's configure entry is different*/

#define	ACL_VAR_MASTER_SERV_DISABLE		"master_disable"
#define	ACL_VAR_MASTER_SERV_SERVICE		"master_service"
#define	ACL_VAR_MASTER_SERV_TYPE		"master_type"
#define	ACL_VAR_MASTER_SERV_PRIVATE		"master_private"
#define	ACL_VAR_MASTER_SERV_UNPRIV		"master_unpriv"
#define	ACL_VAR_MASTER_SERV_CHROOT		"master_chroot"
#define	ACL_VAR_MASTER_SERV_WAKEUP		"master_wakeup"
#define	ACL_VAR_MASTER_SERV_LOG			"master_log"
#define	ACL_VAR_MASTER_SERV_COMMAND		"master_command"
#define ACL_VAR_MASTER_SERV_CMDEXT		"master_cmdext"
#define	ACL_VAR_MASTER_SERV_ARGS		"master_args"
#define	ACL_VAR_MASTER_SERV_OWNER		"master_owner"
#define	ACL_VAR_MASTER_SERV_ENV			"master_env"
#define	ACL_VAR_MASTER_NOTIFY_ADDR		"master_notify_addr"
#define	ACL_VAR_MASTER_NOTIFY_RECIPIENTS	"master_notify_recipients"

#define	ACL_DEF_MASTER_SERV_MAX_QLEN		"128"
#define	ACL_VAR_MASTER_SERV_MAX_QLEN		"master_backlog"

#define	ACL_DEF_MASTER_SERV_MAX_PROC		"5"
#define	ACL_VAR_MASTER_SERV_MAX_PROC		"master_maxproc"

#define	ACL_DEF_MASTER_SERV_PREFORK_PROC	"0"
#define	ACL_VAR_MASTER_SERV_PREFORK_PROC	"master_prefork"

#define	ACL_VAR_MASTER_SERV_DEFER_ACCEPT	"master_defer_accept"
#define	ACL_DEF_MASTER_SERV_DEFER_ACCEPT	"0"

#define ACL_VAR_MASTER_SERV_REUSEPORT		"master_reuseport"
#define ACL_VAR_MASTER_SERV_FASTOPEN		"master_fastopen"
#define ACL_VAR_MASTER_SERV_NBLOCK		"master_nonblock"
#define ACL_VAR_MASETR_SERV_STOP_KILL		"master_stop_kill"
#define ACL_VAR_MASTER_SERV_STOP_WAIT		"master_stop_wait"

#define ACL_VAR_MASTER_SERV_VERSION		"master_version"
#define ACL_VAR_MASTER_SERV_CHECK_FDS		"master_check_fds"
#define ACL_VAR_MASTER_SERV_CHECK_MEM		"master_check_mem"
#define ACL_VAR_MASTER_SERV_CHECK_CPU		"master_check_cpu"
#define ACL_VAR_MASTER_SERV_CHECK_IO		"master_check_io"
#define ACL_VAR_MASTER_SERV_CHECK_LIMITS	"master_check_limits"
#define ACL_VAR_MASTER_SERV_CHECK_NET		"master_check_net"

/**
 * master_params.c
 */
void acl_master_params_load(const char *pathname);

/**
 * master_vars.c
 */
extern acl_pthread_pool_t *acl_var_master_thread_pool;
void acl_master_vars_init(int buf_size, int rw_timeout);
void acl_master_vars_end(void);

#ifdef	__cplusplus
}
#endif

#endif
