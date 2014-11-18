/* System libraries. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include "stdlib/acl_msg.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef	ACL_UNIX

#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>

/* Application-specific. */

#include "stdlib/acl_iostuff.h"

 /*
  * 44BSD compatibility.
  */
#ifndef RLIMIT_NOFILE
#ifdef RLIMIT_OFILE
#define RLIMIT_NOFILE RLIMIT_OFILE
#endif
#endif

/* acl_open_limit - set/query file descriptor limit */

#include <stdio.h>

int acl_open_limit(int limit)
{
#ifdef RLIMIT_NOFILE
	struct rlimit rl;
	int rlim_cur = -1;

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0)
		return -1;

	rlim_cur = (int) rl.rlim_cur;
	if (limit > 0) {
		if (limit > (int) rl.rlim_max)
			rl.rlim_cur = rl.rlim_max;
		else
			rl.rlim_cur = limit;
		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
			acl_msg_warn("setrlimit error: %s, %d",
				acl_last_serror(), (int) limit);
			return rlim_cur;
		}
		else
			return (int) rl.rlim_cur;
	} else if (rl.rlim_max > rl.rlim_cur) {
		rlim_cur = (int) rl.rlim_cur;
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
			acl_msg_warn("setrlimit error: %s, %d",
				acl_last_serror(), (int) limit);
			return rlim_cur;
		}

		return (int) rl.rlim_cur;
	} else
		return (int) rl.rlim_cur;

#else
	(void) limit;
	return getdtablesize();
#endif
}

#endif /* end ACL_UNIX */

