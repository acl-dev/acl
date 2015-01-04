/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/unix/acl_safe_open.h"

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
		(struct stat *) 0, (uid_t) -1, (uid_t) -1, why)) == 0)
	{
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
	if (acl_write_buf(fd, buf, len, timeout) < 0)
		if (acl_msg_verbose)
			acl_msg_warn("%s: write %s: %s",
				myname, service, strerror(errno));

	/*
	 * Disconnect.
	 */
	if (acl_vstream_fclose(fp))
		if (acl_msg_verbose)
			acl_msg_warn("%s: close %s: %s",
				myname, service, strerror(errno));
	return 0;
}
#endif /* ACL_UNIX */

#ifdef TEST

/*
 * Set up a FIFO listener, and keep triggering until the listener becomes
 * idle, which should never happen.
 */
#include <signal.h>
#include <stdlib.h>

#include "acl_events.h"
#include "acl_listen.h"

#define TEST_FIFO	"test-fifo"

int     trig_count;
int     wakeup_count;

static void cleanup(void)
{
	unlink(TEST_FIFO);
	exit(1);
}

static void handler(int sig)
{
	acl_msg_fatal("got signal %d after %d triggers %d wakeups",
		sig, trig_count, wakeup_count);
}

static void read_event(int unused_event, char *context)
{
	int     fd = (int) context;
	char    ch;

	wakeup_count++;

	if (read(fd, &ch, 1) != 1)
		acl_msg_fatal("read %s: %m", TEST_FIFO);
}

int     main(int unused_argc, char **unused_argv)
{
	int     listen_fd;

	listen_fd = acl_fifo_listen(TEST_FIFO, 0600, ACL_NON_BLOCKING);
	acl_msg_cleanup(cleanup);
	acl_event_enable_read(listen_fd, read_event, (char *) listen_fd);
	signal(SIGINT, handler);
	signal(SIGALRM, handler);
	for (;;) {
		alarm(10);
		if (acl_fifo_trigger(TEST_FIFO, "", 1, 0) < 0)
			acl_msg_fatal("trigger %s: %m", TEST_FIFO);
		trig_count++;
		if (acl_fifo_trigger(TEST_FIFO, "", 1, 0) < 0)
			acl_msg_fatal("trigger %s: %m", TEST_FIFO);
		trig_count++;
		if (acl_fifo_trigger(TEST_FIFO, "", 1, 0) < 0)
			acl_msg_fatal("trigger %s: %m", TEST_FIFO);
		trig_count++;
		acl_event_loop(-1);
		acl_event_loop(-1);
		acl_event_loop(-1);
	}

	return 0;
}

#endif

