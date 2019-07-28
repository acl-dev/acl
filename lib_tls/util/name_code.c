#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include "name_code.h"

/* name_code - look up code by name */

int     name_code(const NAME_CODE *table, int flags, const char *name)
{
	const NAME_CODE *np;
	int     (*lookup) (const char *, const char *);

	if (flags & NAME_CODE_FLAG_STRICT_CASE)
		lookup = strcmp;
	else
		lookup = strcasecmp;

	for (np = table; np->name; np++)
		if (lookup(name, np->name) == 0)
			break;
	return (np->code);
}

/* str_name_code - look up name by code */

const char *str_name_code(const NAME_CODE *table, int code)
{
	const NAME_CODE *np;

	for (np = table; np->name; np++)
		if (code == np->code)
			break;
	return (np->name);
}
