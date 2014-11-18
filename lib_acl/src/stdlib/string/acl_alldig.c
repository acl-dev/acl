/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_stringops.h"

#endif

/* acl_alldig - return true if string is all digits */

int acl_alldig(const char *string)
{
	const char *cp;

	if (*string == 0)
		return (0);
	for (cp = string; *cp != 0; cp++)
		if (!ACL_ISDIGIT(*cp))
			return (0);
	return (1);
}
