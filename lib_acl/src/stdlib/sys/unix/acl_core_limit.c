#include "StdAfx.h"

#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#endif

#ifdef	ACL_UNIX

#include <sys/time.h>
#include <sys/resource.h>
#if !defined(ACL_MACOSX) && !defined(ACL_SUNOS5) && !defined(ACL_FREEBSD)
# include <sys/prctl.h>
#endif
#include "stdlib/acl_msg.h"
#include "stdlib/unix/acl_core_limit.h"

void acl_set_core_limit(unsigned long long int max)
{
	const char *myname = "set_limit";
	struct rlimit rlim, rlim_new;

#if !defined(ACL_MACOSX) && !defined(ACL_SUNOS5) && !defined(ACL_FREEBSD)
	if (prctl(PR_SET_DUMPABLE, 1) < 0) {
		acl_msg_warn("%s(%d): prctl error(%s)",
			myname, __LINE__, acl_last_serror());
	}
#endif

	if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
		if (max == 0)
			max = RLIM_INFINITY;
		rlim_new.rlim_cur = rlim_new.rlim_max = max;
		if (setrlimit(RLIMIT_CORE, &rlim_new) != 0) {
			/* failed. try raising just to the old max */
			rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
			if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
				acl_msg_warn("%s(%d): can't set core limit: %s",
					myname, __LINE__, acl_last_serror());
		}
	} else {
		if (max == 0)
			max = RLIM_INFINITY;
		rlim.rlim_cur = max;
		rlim.rlim_max = max;
		if (setrlimit(RLIMIT_CORE, &rlim) != 0)
			acl_msg_warn("%s(%d): can't set core limit: %s",
				myname, __LINE__, acl_last_serror());
	}
}

#endif
