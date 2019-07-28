/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <signal.h>

/* Utility library.*/

#include "posix_signals.h"

#ifdef MISSING_SIGSET_T

int sigemptyset(sigset_t *m)
{
	return *m = 0;
}

int sigaddset(sigset_t *set, int signum)
{
	*set |= sigmask(signum);
	return 0;
}

int sigprocmask(int how, sigset_t *set, sigset_t *old)
{
	int previous;

	if (how == SIG_BLOCK)
		previous = sigblock(*set);
	else if (how == SIG_SETMASK)
		previous = sigsetmask(*set);
	else if (how == SIG_UNBLOCK) {
		int     m = sigblock(0);

		previous = sigsetmask(m & ~*set);
	} else {
		acl_set_error(EINVAL);
		return -1;
	}

	if (old)
		*old = previous;
	return 0;
}

#endif

#ifdef MISSING_SIGACTION

static struct sigaction actions[NSIG] = {};

static int sighandle(int signum)
{
	if (signum == SIGCHLD) {
		/* XXX If the child is just stopped, don't invoke the handler */
	}
	actions[signum].sa_handler(signum);
}

int sigaction(int sig, struct sigaction *act, struct sigaction *oact)
{
	static int initialized = 0;

	if (!initialized) {
		int     i;

		for (i = 0; i < NSIG; i++)
			actions[i].sa_handler = SIG_DFL;
		initialized = 1;
	}
	if (sig <= 0 || sig >= NSIG) {
		acl_set_error(EINVAL);
		return -1;
	}
	if (oact)
		*oact = actions[sig];

	{
		struct sigvec mine = {
			sighandle, act->sa_mask,
			act->sa_flags & SA_RESTART ? SV_INTERRUPT : 0
		};

		if (sigvec(sig, &mine, NULL))
			return -1;
	}

	actions[sig] = *act;
	return 0;
}

#endif

#endif /* ACL_UNIX*/
