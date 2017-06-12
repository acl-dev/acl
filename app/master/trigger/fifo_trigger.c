#include "stdafx.h"

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

/* Process manager. */

#include "trigger.h"

/* acl_fifo_trigger - wakeup fifo server */

int acl_fifo_trigger(ACL_EVENT *eventp_unused acl_unused, const char *service,
	const char *buf, int len, int timeout)
{
	const char *myname = "acl_fifo_trigger";
	static ACL_VSTRING *why;
	ACL_VSTREAM *fp;
	int     fd;

	if (why == 0)
		why = acl_vstring_alloc(1);

	/*
	 * Write the request to the service fifo. According to POSIX, the open
	 * shall always return immediately, and shall return an error when no
	 * process is reading from the FIFO.
	 * 
	 * Use safe_open() so that we don't follow symlinks, and so that we don't
	 * open files with multiple hard links. We're not (yet) going to bother
	 * the caller with safe_open() specific quirks such as the why argument.
	 */
	if ((fp = acl_safe_open(service, O_WRONLY | O_NONBLOCK, 0,
		(struct stat *) 0, (uid_t) -1, (uid_t) -1, why)) == 0) {

		if (acl_msg_verbose)
			acl_msg_info("%s: open %s: %s",
				myname, service, acl_vstring_str(why));
		return -1;
	}

	fd = ACL_VSTREAM_FILE(fp);

	/*
	 * Write the request...
	 */
	acl_non_blocking(fd, timeout > 0 ? ACL_NON_BLOCKING : ACL_BLOCKING);
	if (acl_write_buf(fd, buf, len, timeout) < 0 && acl_msg_verbose)
		acl_msg_warn("%s: write %s: %s",
			myname, service, strerror(errno));

	/*
	 * Disconnect.
	 */
	if (acl_vstream_fclose(fp) && acl_msg_verbose)
		acl_msg_warn("%s: close %s: %s",
			myname, service, strerror(errno));
	return 0;
}
