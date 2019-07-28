#ifndef __ACL_POSIX_SIGNALS_H_INCLUDED__
#define __ACL_POSIX_SIGNALS_H_INCLUDED__

#ifdef  __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"

#ifdef ACL_UNIX

 /*
  * Compatibility interface.
  */

#ifdef MISSING_SIGSET_T

typedef int sigset_t;

enum {
	SIG_BLOCK,
	SIG_UNBLOCK,
	SIG_SETMASK
};

extern int sigemptyset(sigset_t *);
extern int sigaddset(sigset_t *, int);
extern int sigprocmask(int, sigset_t *, sigset_t *);

#endif

#ifdef MISSING_SIGACTION

struct sigaction {
	void    (*sa_handler) ();
	sigset_t sa_mask;
	int     sa_flags;
};

/* Possible values for sa_flags.  Or them to set multiple.  */
enum {
	SA_RESTART,
	SA_NOCLDSTOP = 4			/* drop the = 4.  */
};

extern int sigaction(int, struct sigaction *, struct sigaction *);

#endif

#endif /* ACL_UNIX */

#ifdef  __cplusplus
}
#endif

#endif
