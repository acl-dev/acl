/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

/* Utility library. */

#include "posix_signals.h"
#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_timed_wait.h"

/* Application-specific. */

static int timed_wait_expired;

/* timed_wait_alarm - timeout handler */

static void timed_wait_alarm(int unused_sig acl_unused)
{
	/*
	 * WARNING WARNING WARNING.
	 * 
	 * This code runs at unpredictable moments, as a signal handler.
	 * This code is here only so that we can break out of waitpid().
	 * Don't put any code here other than for setting a global flag.
	 */
	timed_wait_expired = 1;
}

/* timed_waitpid - waitpid with time limit */

int acl_timed_waitpid(pid_t pid, ACL_WAIT_STATUS_T *statusp, int options,
	int time_limit)
{
	const char *myname = "timed_waitpid";
	struct sigaction action;
	struct sigaction old_action;
	int     time_left;
	int     wpid;
	char    tbuf[256];

	/*
	 * Sanity checks.
	 */
	if (time_limit <= 0)
		acl_msg_panic("%s: bad time limit: %d", myname, time_limit);

	/*
	 * Set up a timer.
	 */
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = timed_wait_alarm;
	if (sigaction(SIGALRM, &action, &old_action) < 0)
		acl_msg_fatal("%s: sigaction(SIGALRM): %s", myname,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	timed_wait_expired = 0;
	time_left = alarm(time_limit);

	/*
	 * Wait for only a limited amount of time.
	 */
	if ((wpid = waitpid(pid, statusp, options)) < 0 && timed_wait_expired)
		acl_set_error(ETIMEDOUT);

	/*
	 * Cleanup.
	 */
	alarm(0);
	if (sigaction(SIGALRM, &old_action, (struct sigaction *) 0) < 0)
		acl_msg_fatal("%s: sigaction(SIGALRM): %s", myname,
			acl_last_strerror(tbuf, sizeof(tbuf)));
	if (time_left)
		alarm(time_left);

	return (wpid);
}
#endif /* ACL_UNIX */
