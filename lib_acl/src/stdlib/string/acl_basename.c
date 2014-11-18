/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_stringops.h"

#endif

/* acl_safe_basename - skip directory prefix */

const char *acl_safe_basename(const char *path)
{
	const char   *result;

	if ((result = strrchr(path, '/')) == NULL
		&& (result = strrchr(path, '\\')) == NULL)
	{
		result = (const char *) path;
	} else
		result += 1;
	return (result);
}


