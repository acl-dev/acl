/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef	ACL_UNIX

# ifdef	ACL_SUNOS5
#include <stropts.h>
# endif

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
/*
# ifndef ACL_FREEBSD
#  include <stropts.h>
# endif
*/
#include <sys/types.h>
#include <fcntl.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_connect.h"

/* acl_stream_connect - connect to stream listener */

#ifdef SUNOS5
int acl_stream_connect(const char *path, int block_mode, int unused_timeout)
{
	const char *myname = "acl_stream_connect";

#ifdef ACL_FREEBSD
	path = path;
	block_mode = block_mode;
	(void) unused_timeout;

	acl_msg_fatal("%s(%d): not support!", myname, __LINE__);
	return -1;
#else
	int     pair[2];
	int     fifo;

	(void) unused_timeout;

	/*
	 * The requested file system object must exist, otherwise we can't reach
	 * the server.
	 */
	if ((fifo = open(path, O_WRONLY | O_NONBLOCK, 0)) < 0) {
		return -1;
	}

	/*
	 * Create a pipe, and send one pipe end to the server.
	 */
	if (pipe(pair) < 0) {
		acl_msg_fatal("%s: pipe: %s", myname, acl_last_serror());
	}
	if (ioctl(fifo, I_SENDFD, pair[1]) < 0) {
		acl_msg_fatal("%s: send file descriptor: %s",
			myname, acl_last_serror());
	}
	close(pair[1]);

	/*
	 * This is for {unix,inet}_connect() compatibility.
	 */
	if (block_mode == ACL_NON_BLOCKING) {
		acl_non_blocking(pair[0], ACL_NON_BLOCKING);
	}

	/*
	 * Cleanup.
	 */
	close(fifo);

	/*
	 * Keep the other end of the pipe.
	 */
	return pair[0];
#endif /* ACL_FREEBSD */
}
#endif

#endif /* ACL_UNIX */
