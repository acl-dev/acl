#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

/* System libraries */

#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_split_at.h"

#endif

/* acl_split_at - break string at first delimiter, return remainder */

char  *acl_split_at(char *s, int delimiter)
{
	char   *cp;

	if ((cp = strchr(s, delimiter)) != 0)
		*cp++ = 0;
	return cp;
}

/* acl_split_at_right - break string at last delimiter, return remainder */

char  *acl_split_at_right(char *s, int delimiter)
{
	char   *cp;

	if ((cp = strrchr(s, delimiter)) != 0)
		*cp++ = 0;
	return cp;
}
