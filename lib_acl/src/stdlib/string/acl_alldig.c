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

int acl_alldig(const char *s)
{
	if (s == NULL || *s == 0)
		return 0;
	for (; *s != 0; s++)
		if (!ACL_ISDIGIT(*s))
			return 0;
	return 1;
}

int acl_is_double(const char *s)
{
	if (s == NULL || *s == 0)
		return 0;
	if (*s == '-' || *s == '+')
		s++;
	if (*s == 0 || *s == '.')
		return 0;

	while (*s != 0) {
		if (*s == '.') {
			s++;
			if (*s == 0)
				return 0;
			break;
		}
		if (!ACL_ISDIGIT(*s))
			return 0;
		s++;
	}

	if (*s == 0)
		return 1;

	while (*s != 0) {
		if (!ACL_ISDIGIT(*s))
			return 0;
		s++;
	}

	return 1;
}
