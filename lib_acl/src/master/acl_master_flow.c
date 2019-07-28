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

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"

/* Global library. */

#include "master/acl_master_proto.h"
#include "master/acl_master_flow.h"

#define BUFFER_SIZE	1024

int     acl_var_master_flow_pipe[2];

/* acl_master_flow_init - initialize the flow control channel */

void acl_master_flow_init(void)
{
    char   *myname = "acl_master_flow_init";

    if (pipe(acl_var_master_flow_pipe) < 0)
	acl_msg_fatal("%s(%d)->%s: pipe: %s",
		__FILE__, __LINE__, myname, strerror(errno));

    acl_non_blocking(acl_var_master_flow_pipe[0], ACL_NON_BLOCKING);
    acl_non_blocking(acl_var_master_flow_pipe[1], ACL_NON_BLOCKING);

    acl_close_on_exec(acl_var_master_flow_pipe[0], ACL_CLOSE_ON_EXEC);
    acl_close_on_exec(acl_var_master_flow_pipe[1], ACL_CLOSE_ON_EXEC);
}

/* acl_master_flow_get - read N tokens */

int acl_master_flow_get(int len)
{
	const char *myname = "acl_master_flow_get";
	char  buf[BUFFER_SIZE];
	struct stat st;
	int   count;
	ssize_t n = 0;

	/*
	 * Sanity check.
	 */
	if (len <= 0)
		acl_msg_panic("%s: bad length %d", myname, len);

	/*
	 * Silence some wild claims.
	 */
	if (fstat(ACL_MASTER_FLOW_WRITE, &st) < 0)
		acl_msg_fatal("fstat flow pipe write descriptor: %s",
			strerror(errno));

	/*
	 * Read and discard N bytes. XXX AIX read() can return 0 when an open
	 * pipe is empty.
	 */
	for (count = len; count > 0; count -= n) {
		n = read(ACL_MASTER_FLOW_READ, buf,
			count > BUFFER_SIZE ? BUFFER_SIZE : count);
		if (n <= 0)
			return -1;
	}

	if (acl_msg_verbose)
		acl_msg_info("%s: %d %d", myname, len, len - count);
	return len - count;
}

/* acl_master_flow_put - put N tokens */

int acl_master_flow_put(int len)
{
	const char *myname = "acl_master_flow_put";
	char  buf[BUFFER_SIZE];
	int   count;
	ssize_t n = 0;

	/* Sanity check. */
	if (len <= 0)
		acl_msg_panic("%s: bad length %d", myname, len);

	/* Write or discard N bytes. */
	memset(buf, 0, len > BUFFER_SIZE ? BUFFER_SIZE : len);

	for (count = len; count > 0; count -= n) {
		n = write((int) ACL_MASTER_FLOW_WRITE, buf,
			(size_t) count > BUFFER_SIZE ?
		       		BUFFER_SIZE : (size_t) count);
		if (n < 0)
			return -1;
	}

	if (acl_msg_verbose)
		acl_msg_info("%s: %d %d", myname, len, len - count);
	return len - count;
}

/* acl_master_flow_count - return number of available tokens */

int acl_master_flow_count(void)
{
	const char *myname = "acl_master_flow_count";
	int   count;

	if ((count = acl_peekfd(ACL_MASTER_FLOW_READ)) < 0)
		acl_msg_warn("%s: %s", myname, strerror(errno));
	return count;
}

#endif /* ACL_UNIX */
#endif /* ACL_CLIENT_ONLY */
