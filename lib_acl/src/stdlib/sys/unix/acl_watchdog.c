/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <unistd.h>
#include <signal.h>
#include <string.h>
#include "posix_signals.h"

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/unix/acl_watchdog.h"

/* Application-specific. */

 /*
  * Rather than having one timer that goes off when it is too late, we break
  * up the time limit into smaller intervals so that we can deal with clocks
  * that jump occasionally.
  */
#define ACL_WATCHDOG_STEPS	3

 /*
  * UNIX alarms are not stackable, but we can save and restore state, so that
  * watchdogs can at least be nested, sort of.
  */
 struct ACL_WATCHDOG {
	 unsigned timeout;		/* our time resolution */
	 ACL_WATCHDOG_FN action;	/* application routine */
	 char   *context;		/* application context */
	 int     trip_run;		/* number of successive timeouts */
	 ACL_WATCHDOG *saved_watchdog;	/* saved state */
	 struct sigaction saved_action;	/* saved state */
	 unsigned saved_time;		/* saved state */
 };

 /*
  * However, only one watchdog instance can be current, and the caller has to
  * restore state before a prior watchdog instance can be manipulated.
  */
static ACL_WATCHDOG *acl_watchdog_curr;

/* acl_watchdog_event - handle timeout event */

static void acl_watchdog_event(int unused_sig acl_unused)
{
	const char *myname = "acl_watchdog_event";
	ACL_WATCHDOG *wp;

	/*
	 * This routine runs as a signal handler. We should not do anything
	 * that could involve memory allocation/deallocation, but exiting
	 * without proper explanation would be unacceptable.
	 */
	if ((wp = acl_watchdog_curr) == 0)
		acl_msg_panic("%s: no instance", myname);
	if (acl_msg_verbose > 1)
		acl_msg_info("%s: %p %d", myname, (void *) wp, wp->trip_run);
	if (++(wp->trip_run) < ACL_WATCHDOG_STEPS)
		alarm(wp->timeout);
	else {
		if (wp->action)
			wp->action(wp, wp->context);
		else
			acl_msg_fatal("watchdog timeout");
	}
}

/* acl_watchdog_create - create watchdog instance */

ACL_WATCHDOG *acl_watchdog_create(unsigned timeout,
	ACL_WATCHDOG_FN action, char *context)
{
	const char *myname = "acl_watchdog_create";
	struct sigaction sig_action;
	ACL_WATCHDOG *wp;

	wp = (ACL_WATCHDOG *) acl_mymalloc(sizeof(*wp));
	if ((wp->timeout = timeout / ACL_WATCHDOG_STEPS) == 0)
		acl_msg_panic("%s: timeout %d too small", myname, timeout);
	wp->action = action;
	wp->context = context;
	wp->saved_watchdog = acl_watchdog_curr;
	wp->saved_time = alarm(0);
	sigemptyset(&sig_action.sa_mask);
#ifdef SA_RESTART
	sig_action.sa_flags = SA_RESTART;
#else
	sig_action.sa_flags = 0;
#endif
	sig_action.sa_handler = acl_watchdog_event;
	if (sigaction(SIGALRM, &sig_action, &wp->saved_action) < 0)
		acl_msg_fatal("%s: sigaction(SIGALRM): %s",
			myname, acl_last_serror());
	if (acl_msg_verbose > 1)
		acl_msg_info("%s: %p %d", myname, (void *) wp, timeout);
	return (acl_watchdog_curr = wp);
}

/* acl_watchdog_destroy - destroy watchdog instance, restore state */

void    acl_watchdog_destroy(ACL_WATCHDOG *wp)
{
	const char *myname = "acl_watchdog_destroy";

	acl_watchdog_stop(wp);
	acl_watchdog_curr = wp->saved_watchdog;
	if (sigaction(SIGALRM, &wp->saved_action, (struct sigaction *) 0) < 0)
		acl_msg_fatal("%s: sigaction(SIGALRM): %s",
			myname, acl_last_serror());
	if (wp->saved_time)
		alarm(wp->saved_time);
	acl_myfree(wp);
	if (acl_msg_verbose > 1)
		acl_msg_info("%s: %p", myname, (void *) wp);
}

/* acl_watchdog_start - enable watchdog timer */

void    acl_watchdog_start(ACL_WATCHDOG *wp)
{
	const char *myname = "acl_watchdog_start";

	if (wp != acl_watchdog_curr)
		acl_msg_panic("%s: wrong watchdog instance", myname);
	wp->trip_run = 0;
	alarm(wp->timeout);
	if (acl_msg_verbose > 1)
		acl_msg_info("%s: %p", myname, (void *) wp);
}

/* acl_watchdog_stop - disable watchdog timer */

void    acl_watchdog_stop(ACL_WATCHDOG *wp)
{
	const char *myname = "acl_watchdog_stop";

	if (wp != acl_watchdog_curr)
		acl_msg_panic("%s: wrong watchdog instance", myname);
	alarm(0);
	if (acl_msg_verbose > 1)
		acl_msg_info("%s: %p", myname, (void *) wp);
}

/* acl_watchdog_pat - pat the dog so it stays quiet */

void    acl_watchdog_pat(void)
{
	const char *myname = "acl_watchdog_pat";

	if (acl_watchdog_curr)
		acl_watchdog_curr->trip_run = 0;
	if (acl_msg_verbose > 1)
		acl_msg_info("%s: %p", myname, (void *) acl_watchdog_curr);
}

#endif /* ACL_UNIX */
