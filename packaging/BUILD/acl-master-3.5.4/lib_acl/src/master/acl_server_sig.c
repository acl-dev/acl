#include "StdAfx.h"

#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Application-specific. */

#include "stdlib/acl_msg.h"
#include "master/acl_server_api.h"

#endif

#ifndef ACL_CLIENT_ONLY

#ifdef ACL_WINDOWS
#include <signal.h>
#endif

/* Local stuff. */

int   acl_var_server_gotsighup = 0;

/* master_sighup - register arrival of hangup signal */

static void server_sighup(int sig)
{
	/*
	 * WARNING WARNING WARNING.
	 * 
	 * This code runs at unpredictable moments, as a signal handler.
	 * Don't put any code here other than for setting a global flag.
	 */
	acl_var_server_gotsighup = sig;
}

/* acl_server_sighup_setup - set up SIGHUP signal handlers */

void acl_server_sighup_setup(void)
{
#ifdef ACL_WINDOWS
	signal(1, server_sighup);
#else
	struct sigaction action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

#ifdef SA_RESTART
	action.sa_flags |= SA_RESTART;
#endif
	action.sa_handler = server_sighup;

	if (sigaction(SIGHUP, &action, (struct sigaction *) 0) < 0)
		acl_msg_fatal("%s: sigaction(%d): %s",
			__FUNCTION__, SIGHUP, strerror(errno));
#endif
}

#ifdef ACL_UNIX
static void server_sigterm(int sig acl_unused)
{
	int i = 0, max = 1024;

	/*
	acl_msg_info("%s(%d), %s: got SIGTERM, close from %d to %d",
		__FILE__, __LINE__, __FUNCTION__, i, max);
	*/

	for (; i < max; i++)
		close(i);
	acl_doze(100); /* just sleep 100 ms for release fd by OS system ! */
	_exit(0);
}

#endif

void acl_server_sigterm_setup(void)
{
#ifdef ACL_UNIX
	struct sigaction action;

	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;

#ifdef SA_RESTART
	action.sa_flags |= SA_RESTART;
#endif
	action.sa_handler = server_sigterm;

	if (sigaction(SIGTERM, &action, (struct sigaction *) 0) < 0)
		acl_msg_fatal("%s: sigaction(%d): %s",
			__FUNCTION__, SIGTERM, strerror(errno));
#endif
}

#endif /* ACL_CLIENT_ONLY */
