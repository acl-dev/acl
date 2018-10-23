#ifndef	__MASTER_INCLUDE_H__
#define	__MASTER_INCLUDE_H__

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct ACL_MASTER_PROC ACL_MASTER_PROC;
typedef void (*STATUS_CALLBACK)(ACL_MASTER_PROC*, int, void*);

typedef struct ACL_MASTER_NV {
	char *name;
	char *value;
} ACL_MASTER_NV;

typedef struct ACL_MASTER_ADDR {
	int   type;
	char *addr;
} ACL_MASTER_ADDR;

 /*
  * Server processes that provide the same service share a common "listen"
  * socket to accept connection requests, and share a common pipe to the
  * master process to send status reports. Server processes die voluntarily
  * when idle for a configurable amount of time, or after servicing a
  * configurable number of requests; the master process spawns new processes
  * on demand up to a configurable concurrency limit and/or periodically.
  */
typedef struct ACL_MASTER_SERV {
	int      flags;			/* status, features, etc. */
	char    *name;			/* service endpoint name */
	char    *owner;			/* service running privilege if not null*/
	long     start;			/* service start running time */
	int      type;			/* UNIX-domain, INET, etc. */
	int      wakeup_time;		/* wakeup interval */
	unsigned inet_flags;		/* listen/bind inet flags */
	int     *listen_fds;		/* incoming requests */
	int      listen_fd_count;	/* nr of descriptors */
	int      defer_accept;		/* accept timeout if no data from client */
	int      max_qlen;		/* max listening qlen */
	int      max_proc;		/* upper bound on # processes */
	int      prefork_proc;		/* prefork processes */
	//char    *command;		/* command in configure */
	char    *cmdext;		/* extname of the command */
	char    *path;			/* command pathname */
	char    *conf;			/* service configure filepath */
	char    *notify_addr;		/* warning address when not null */
	char    *notify_recipients;	/* users warned to */
	char    *version;		/* the service's version */
	int      avail_proc;		/* idle processes */
	int      total_proc;		/* number of processes */
	int      throttle_delay;	/* failure recovery parameter */
	int      status_fd[2];		/* child status reports */
	ACL_ARGV     *args;		/* argument vector */
	ACL_ARRAY    *addrs;		/* in which ACL_MASTER_ADDR save */
	ACL_ARRAY    *children_env;	/* the env array of the children */
	ACL_VSTREAM **listen_streams;	/* multi-listening stream */
	ACL_VSTREAM  *status_reader;	/* status stream */
	ACL_RING      children;		/* linkage of children */
	struct ACL_MASTER_SERV *next;	/* linkage of serv */

	char check_fds;
	char check_mem;
	char check_cpu;
	char check_io;
	char check_limits;
	char check_net;

	STATUS_CALLBACK  callback;
	void            *ctx;
} ACL_MASTER_SERV;

#define ACL_MASTER_CHILDREN_SIZE(s)     acl_ring_size(&s->children)

 /*
  * Per-service flag bits. We assume trouble when a child process terminates
  * before completing its first request: either the program is defective,
  * some configuration is wrong, or the system is out of resources.
  */
#define ACL_MASTER_FLAG_THROTTLE	(1<<0)	/* we're having trouble */
#define ACL_MASTER_FLAG_MARK		(1<<1)	/* garbage collection support */
#define ACL_MASTER_FLAG_CONDWAKE	(1<<2)	/* wake up if actually used */
#define	ACL_MASTER_FLAG_RELOADING	(1<<3)	/* the service is reloading */
#define ACL_MASTER_FLAG_STOPPING	(1<<4)	/* the service is stopping */
#define ACL_MASTER_FLAG_KILLED          (1<<5)  /* the service is killed */
#define ACL_MASTER_FLAG_STOP_KILL	(1<<6)  /* the service can be killed on stopping */
#define ACL_MASTER_FLAG_STOP_WAIT	(1<<7)  /* master waiting service exited */

#define ACL_MASTER_THROTTLED(f)		((f)->flags & ACL_MASTER_FLAG_THROTTLE)
#define ACL_MASTER_STOPPING(f)		((f)->flags & ACL_MASTER_FLAG_STOPPING)
#define ACL_MASTER_KILLED(f)		((f)->flags & ACL_MASTER_FLAG_KILLED)

#define ACL_MASTER_LIMIT_OK(limit, count) ((limit) == 0 || ((count) < (limit)))

 /*
  * Service types.
  */
#define	ACL_MASTER_SERV_TYPE_NULL	0	/* invalid type */
#define ACL_MASTER_SERV_TYPE_UNIX	1	/* AF_UNIX domain socket */
#define ACL_MASTER_SERV_TYPE_INET	2	/* AF_INET domain socket */
#define ACL_MASTER_SERV_TYPE_FIFO	3	/* fifo (named pipe) */
#define	ACL_MASTER_SERV_TYPE_SOCK	4	/* AF_UNIX/AF_INET socket */
#define	ACL_MASTER_SERV_TYPE_UDP	5	/* AF_INET UDP socket */

 /*
  * Default process management policy values. This is only the bare minimum.
  * Most policy management is delegated to child processes. The process
  * manager runs at high privilege level and has to be kept simple.
  */
#define ACL_MASTER_DEF_MIN_IDLE	1	/* preferred # of idle processes */

 /*
  * Structure of child process.
  */
typedef int ACL_MASTER_PID;		/* pid is key into binhash table */

typedef struct ACL_MASTER_PROC {
	ACL_RING me;			/* linked in serv's children */
	unsigned gen;			/* child generation number */
	int      avail;			/* availability */
	long     start;			/* start time of the process */
	int      use_count;		/* number of service requests */
	ACL_MASTER_PID   pid;		/* child process id */
	ACL_MASTER_SERV *serv;		/* parent linkage */
} ACL_MASTER_PROC;

 /*
  * Other manifest constants.
  */
#define ACL_MASTER_BUF_LEN	2048	/* logical config line length */

 /*
  * master_ent.c
  */
extern void acl_set_master_service_path(const char *);
extern void acl_master_ent_begin(void);
extern void acl_master_ent_end(void);
extern void acl_master_ent_print(ACL_MASTER_SERV *);
extern ACL_MASTER_SERV *acl_master_ent_get(void);
extern ACL_MASTER_SERV *acl_master_ent_load(const char*);
extern void acl_master_ent_free(ACL_MASTER_SERV *);
extern int  acl_master_same_name(ACL_MASTER_SERV *serv, const char *name);
extern ACL_MASTER_SERV *acl_master_ent_find(const char *path);

 /*
  * master_conf.c
  */
extern void acl_master_start_services(void);
extern void acl_master_main_config(void);
extern void acl_master_refresh(void);
extern int  acl_master_refresh_service(ACL_MASTER_SERV *entry);

 /*
  * master_service.c
  */
extern ACL_MASTER_SERV *acl_var_master_head;
extern ACL_EVENT *acl_var_master_global_event;
extern void acl_master_service_init(void);
extern int  acl_master_service_start(ACL_MASTER_SERV *);
extern void acl_master_service_kill(ACL_MASTER_SERV *);
extern void acl_master_service_stop(ACL_MASTER_SERV *serv);
extern void acl_master_service_restart(ACL_MASTER_SERV *);

 /*
  * master_events.c
  */
extern int acl_var_master_gotsighup;
extern int acl_var_master_gotsigchld;
extern int acl_var_master_stopped;
extern void acl_master_sigsetup(void);

 /*
  * master_status.c
  */
extern void acl_master_status_init(ACL_MASTER_SERV *);
extern void acl_master_status_cleanup(ACL_MASTER_SERV *);

 /*
  * master_wakeup.c
  */
extern void acl_master_wakeup_init(ACL_MASTER_SERV *);
extern void acl_master_wakeup_cleanup(ACL_MASTER_SERV *);

 /*
  * master_listen.c
  */
extern int  acl_master_listen_init(ACL_MASTER_SERV *);
extern void acl_master_listen_cleanup(ACL_MASTER_SERV *);

/*
 * msg_prefork.c
 */
extern void acl_master_prefork(ACL_MASTER_SERV *);

 /*
  * master_avail.c
  */
extern void acl_master_avail_listen(ACL_MASTER_SERV *);
extern void acl_master_avail_listen_force(ACL_MASTER_SERV *);
extern void acl_master_avail_cleanup(ACL_MASTER_SERV *);
extern void acl_master_avail_more(ACL_MASTER_SERV *, ACL_MASTER_PROC *);
extern void acl_master_avail_less(ACL_MASTER_SERV *, ACL_MASTER_PROC *);

 /*
  * master_spawn.c
  */
extern struct ACL_BINHASH *acl_var_master_child_table;
extern void acl_master_spawn_init(void);
extern void acl_master_spawn(ACL_MASTER_SERV *);
extern void acl_master_reap_child(void);
extern void acl_master_kill_children(ACL_MASTER_SERV *);
extern void acl_master_delete_all_children(void);
extern void acl_master_signal_children(ACL_MASTER_SERV *serv, int signum,
		int *nsignaled);
extern void acl_master_sighup_children(ACL_MASTER_SERV *serv, int *nsignaled);

 /*
  * master_warning.c
  */
extern void master_warning(const char *notify_addr, const char *recipients,
	const char *path, const char* conf, const char *ver,
	int pid, const char *info);

#ifdef  __cplusplus
}
#endif

#endif
