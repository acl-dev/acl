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
	const char *myname = "acl_open_limit";
	int   rlim_cur = -1;

#ifdef RLIMIT_NOFILE
	struct rlimit rl;

	if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
#ifdef ACL_ANDROID
		return 10240;
#else
		rlim_cur = getdtablesize();
		acl_msg_warn("%s(%d): getrlimit error: %s, use: %d",
			myname, __LINE__, acl_last_serror(), rlim_cur);
		return rlim_cur;
#endif
	}

	if (rl.rlim_max <= 0)
		rl.rlim_max = 204800;
	rlim_cur = (int) rl.rlim_cur;

	if (limit > 0) {
		if (limit > (int) rl.rlim_max)
			rl.rlim_cur = rl.rlim_max;
		else
			rl.rlim_cur = limit;
		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
			acl_msg_warn("%s(%d): setrlimit error: %s, limit: %d,"
				" curr: %d", myname, __LINE__,
				acl_last_serror(), limit, rlim_cur);
			return rlim_cur;
		}
		else
			return (int) rl.rlim_cur;
	} else if (rl.rlim_max > rl.rlim_cur) {
		rlim_cur = (int) rl.rlim_cur;
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_NOFILE, &rl) < 0) {
			acl_msg_warn("%s(%d): setrlimit error: %s,"
				" cur: %d, max: %d", myname, __LINE__,
				acl_last_serror(), (int) rl.rlim_cur,
				(int) rl.rlim_max);
			return rlim_cur;
		}

		return (int) rl.rlim_cur;
	} else
		return (int) rl.rlim_cur;

#else
	rlim_cur = getdtablesize();
	if (rlim_cur < 0)
		acl_msg_error("%s(%d): getdtablesize(%d) < 0, limit: %d",
			myname, __LINE__, rlim_cur, limit);
	return rlim_cur;
#endif
}

#endif /* !ACL_UNIX */

