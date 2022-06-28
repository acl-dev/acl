#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_timeops.h"

#endif

time_t acl_str2time_t(const char *str)
{
	struct tm t;

	if (str == NULL)
		return (0);

	memset(&t, 0, sizeof(t));

	if (sscanf(str, "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday) != 3)
		return (0);

	t.tm_year -= 1900;
	t.tm_mon  -= 1;

	return (mktime(&t));
}

