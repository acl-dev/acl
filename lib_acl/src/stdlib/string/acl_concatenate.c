/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_stringops.h"

#endif

/* acl_concatenate - concatenate null-terminated list of strings */

char   *acl_concatenate(const char *arg0,...)
{
	char   *result;
	va_list ap;
	int     len;
	char   *arg;

	/*
	 * Compute the length of the resulting string.
	 */
	va_start(ap, arg0);
	len = (int) strlen(arg0);
	while ((arg = va_arg(ap, char *)) != 0)
		len += (int) strlen(arg);
	va_end(ap);

	/*
	 * Build the resulting string. Don't care about wasting a CPU cycle.
	 */
	result = (char *) acl_mymalloc(len + 1);
	va_start(ap, arg0);
	strcpy(result, arg0);
	while ((arg = va_arg(ap, char *)) != 0)
		strcat(result, arg);
	va_end(ap);
	return (result);
}
