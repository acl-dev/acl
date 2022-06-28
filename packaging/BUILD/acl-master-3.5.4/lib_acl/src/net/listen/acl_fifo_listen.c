/* System interfaces. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef	ACL_UNIX

#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "net/acl_listen.h"

#define BUF_LEN	100

/* acl_fifo_listen - create fifo listener */

int acl_fifo_listen(const char *path, int permissions, int block_mode)
{
	char   *myname = "acl_fifo_listen";
	char    buf[BUF_LEN], tbuf[256];
	static int open_mode = 0;
	struct stat st;
	int     fd;
	int     count;

	/*
	 * Create a named pipe (fifo). Do whatever we can so we don't run into
	 * trouble when this process is restarted after crash.  Make sure that
	 * we open a fifo and not something else, then change permissions to
	 * what we wanted them to be, because mkfifo() is subject to umask
	 * settings. Instead we could zero the umask temporarily before
	 * creating the FIFO, but that would cost even more system calls.
	 * Figure out if the fifo needs to be opened O_RDWR or O_RDONLY. Some
	 * systems need one, some need the other. If we choose the wrong mode,
	 * the fifo will stay readable, causing the program to go into a loop.
	 */
	if (unlink(path) && acl_last_error() != ENOENT) {
		acl_msg_error("%s: remove %s: %s", myname, path,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	if (mkfifo(path, permissions) < 0) {
		acl_msg_error("%s: create fifo %s: %s", myname, path,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		return -1;
	}
	switch (open_mode) {
	case 0:
		if ((fd = open(path, O_RDWR | O_NONBLOCK, 0)) < 0) {
			acl_msg_error("%s: open %s: %s", myname, path,
				acl_last_strerror(tbuf, sizeof(tbuf)));
			return -1;
		}
		if (acl_readable(fd) == 0) {
			open_mode = O_RDWR | O_NONBLOCK;
			break;
		}

		open_mode = O_RDONLY | O_NONBLOCK;
		if (acl_msg_verbose)
			acl_msg_info("open O_RDWR makes fifo readable"
				" - trying O_RDONLY");
		(void) close(fd);
		if ((fd = open(path, open_mode, 0)) < 0) {
			acl_msg_error("%s: open %s: %s", myname, path,
				acl_last_strerror(tbuf, sizeof(tbuf)));
			return -1;
		}
		break;
	default:
		if ((fd = open(path, open_mode, 0)) < 0) {
			acl_msg_error("%s: open %s: %s", myname, path,
				acl_last_strerror(tbuf, sizeof(tbuf)));
			return -1;
		}
		break;
	}

	/*
	 * Make sure we opened a FIFO and skip any cruft that might have
	 * accumulated before we opened it.
	 */
	if (fstat(fd, &st) < 0) {
		acl_msg_error("%s: fstat %s: %s", myname, path,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		close(fd);
		return -1;
	}
	if (S_ISFIFO(st.st_mode) == 0) {
		acl_msg_error("%s: not a fifo: %s", myname, path);
		close(fd);
		return -1;
	}
	if (fchmod(fd, permissions) < 0) {
		acl_msg_error("%s: fchmod %s: %s", myname, path,
			acl_last_strerror(tbuf, sizeof(tbuf)));
		close(fd);
		return -1;
	}
	acl_non_blocking(fd, block_mode);
	while ((count = acl_peekfd(fd)) > 0 && read(fd, buf,
		BUF_LEN < count ? BUF_LEN : count) > 0) {
	}

	return fd;
}

#endif /* ACL_UNIX */

