#ifndef	__ACL_MASTER_PROTO_INCLUDE_H__
#define	__ACL_MASTER_PROTO_INCLUDE_H__

#include "../stdlib/acl_define.h"

#ifndef ACL_CLIENT_ONLY

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef ACL_UNIX

 /*
  * Well-known socket or FIFO directories. The main difference is in file
  * access permissions.
  */
#define ACL_MASTER_CLASS_PUBLIC		"public"
#define ACL_MASTER_CLASS_PRIVATE	"private"

 /*
  * Generic triggers.
  */
#define ACL_TRIGGER_REQ_WAKEUP		'W'	/* wakeup */

 /*
  * Transport names. The master passes the transport name on the command
  * line, and thus the name is part of the master to child protocol.
  */
#define ACL_MASTER_XPORT_NAME_UNIX	"unix"	/* local IPC */
#define ACL_MASTER_XPORT_NAME_FIFO	"fifo"	/* local IPC */
#define ACL_MASTER_XPORT_NAME_INET	"inet"	/* non-local IPC */
#define	ACL_MASTER_XPORT_NAME_SOCK	"sock"	/* inet/unix IPC */
#define	ACL_MASTER_XPORT_NAME_UDP	"udp"	/* udp IPC */
/*#define ACL_MASTER_XPORT_NAME_PASS	"pass"	  local IPC */

 /*
  * Format of a status message sent by a child process to the process
  * manager. Since this is between processes on the same machine we need not
  * worry about byte order and word length.
  */
typedef struct ACL_MASTER_STATUS {
	int      pid;			/* process ID */
	unsigned gen;			/* child generation number */
	int      status;		/* availability */
} ACL_MASTER_STATUS;

#define ACL_MASTER_GEN_NAME	"GENERATION"	/* passed via environment */

#define ACL_MASTER_STAT_TAKEN		0	/* this one is occupied */
#define ACL_MASTER_STAT_AVAIL		1	/* this process is idle */
#define ACL_MASTER_STAT_SIGHUP_OK	2
#define ACL_MASTER_STAT_SIGHUP_ERR	3
#define ACL_MASTER_STAT_START_OK	4
#define ACL_MASTER_STAT_START_ERR	5

int acl_master_notify(int, unsigned, int);	/* encapsulate status msg */

 /*
  * File descriptors inherited from the master process. The flow control pipe
  * is read by receive processes and is written to by send processes. If
  * receive processes get too far ahead they will pause for a brief moment.
  */
#define ACL_MASTER_FLOW_READ	3
#define ACL_MASTER_FLOW_WRITE	4

 /*
  * File descriptors inherited from the master process. All processes that
  * provide a given service share the same status file descriptor, and listen
  * on the same service socket(s). The kernel decides what process gets the
  * next connection. Usually the number of listening processes is small, so
  * one connection will not cause a "thundering herd" effect. When no process
  * listens on a given socket, the master process will. ACL_MASTER_LISTEN_FD is
  * actually the lowest-numbered descriptor of a sequence of descriptors to
  * listen on.
  */
#define ACL_MASTER_STATUS_FD	5	/* shared channel to parent */
#define ACL_MASTER_LISTEN_FD	6	/* accept connections here */

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif /* ACL_CLIENT_ONLY */
#endif
