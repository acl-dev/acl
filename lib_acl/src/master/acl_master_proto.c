/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifndef ACL_CLIENT_ONLY
#ifdef ACL_UNIX

#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Utility library. */

#include "stdlib/acl_msg.h"

/* Global library. */

#include "master/acl_master_proto.h"

int acl_master_notify(int pid, unsigned generation, int status)
{
	char   *myname = "acl_master_notify";
	ACL_MASTER_STATUS stat_buf;

	/*
	 * We use a simple binary protocol to minimize security risks.
	 * Since this is local IPC, there are no byte order or word
	 * length issues. The server treats this information as gossip,
	 * so sending a bad PID or a bad status code will only have
	 * amusement value.
	 */
	stat_buf.pid    = pid;
	stat_buf.gen    = generation;
	stat_buf.status = status;

	if (write(ACL_MASTER_STATUS_FD, (char *) &stat_buf, sizeof(stat_buf))
	    != sizeof(stat_buf)) {
		acl_msg_warn("%s(%d), %s: status %d, error %s",
			__FILE__, __LINE__, myname, status, strerror(errno));
		return -1;
	} else if (acl_msg_verbose)
		acl_msg_info("%s(%d)->%s: OK, status %d, pid = %d",
			__FILE__, __LINE__, myname, status, pid);

	return 0;
}

#endif /* ACL_UNIX */

#endif /* ACL_CLIENT_ONLY */
