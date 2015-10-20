#include "StdAfx.h"
#ifndef	ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>
#include <string.h>
#include "stdlib/acl_stringops.h"

#endif

/* acl_split_nameval - split text into name and value */

const char *acl_split_nameval(char *buf, char **name, char **value)
{
	char   *np;				/* name substring */
	char   *vp;				/* value substring */
	char   *cp;
	char   *ep;

	/*
	 * Ugly macros to make complex expressions less unreadable.
	 */
#define SKIP(start, var, cond) \
	for (var = start; *var && (cond); var++) {}

#define TRIM(s) { \
	char *p; \
	for (p = (s) + strlen(s); p > (s) && ACL_ISSPACE(p[-1]); p--) {} \
	*p = 0; \
}

	SKIP(buf, np, ACL_ISSPACE(*np));		/* find name begin */
	if (*np == 0)
		return ("missing attribute name");
	SKIP(np, ep, !ACL_ISSPACE(*ep) && *ep != '=');	/* find name end */
	SKIP(ep, cp, ACL_ISSPACE(*cp));			/* skip blanks before '=' */
	if (*cp != '=')				/* need '=' */
		return ("missing '=' after attribute name");
	*ep = 0;				/* terminate name */
	cp++;					/* skip over '=' */
	SKIP(cp, vp, ACL_ISSPACE(*vp));		/* skip leading blanks */
	TRIM(vp);				/* trim trailing blanks */
	*name = np;
	*value = vp;
	return (NULL);
}

