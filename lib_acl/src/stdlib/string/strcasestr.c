#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mystring.h"
#endif

#include "../charmap.h"

char *acl_rstrstr(const char *haystack, const char *needle)
{
	unsigned char *cp, *haystack_end;
	const unsigned char *np = 0, *needle_end;

	if (haystack == NULL || *haystack == 0 || needle == NULL || *needle == 0)
		return (NULL);

	haystack_end = (unsigned char *) haystack + strlen(haystack) - 1;
	needle_end = (const unsigned char *) needle + strlen(needle) - 1;

	for (cp = haystack_end; cp >= (unsigned char *) haystack; cp--) {
		if (np) {
			if (*cp == *np) {
				if (--np < (const unsigned char *) needle)
					return ((char *) cp);
			} else
				np = 0;
		}
		if (!np && *cp == *needle_end) {
			np = needle_end - 1;
			if (np < (const unsigned char *) needle)
				return ((char *) cp);
		}
	}

	return (NULL);
}

char *acl_rstrcasestr(const char *haystack, const char *needle)
{
	const unsigned char *cm = maptolower;
	unsigned char *cp, *haystack_end;
	const unsigned char *np = 0, *needle_end;

	if (haystack == NULL || *haystack == 0 || needle == NULL || *needle == 0)
		return (NULL);

	haystack_end = (unsigned char *) haystack + strlen(haystack) - 1;
	needle_end = (const unsigned char *) needle + strlen(needle) - 1;

	for (cp = haystack_end; cp >= (unsigned char *) haystack; cp--) {
		if (np) {
			if (*cp == cm[*np]) {
				if (--np < (const unsigned char *) needle)
					return ((char *) cp);
			} else
				np = 0;
		}
		if (!np && *cp == cm[*needle_end]) {
			np = needle_end - 1;
			if (np < (const unsigned char *) needle)
				return ((char *) cp);
		}
	}

	return (NULL);
}

char *acl_strcasestr(const char *haystack, const char *needle)
{
	const unsigned char *cm = maptolower;
	const unsigned char *np = 0;
	unsigned char *p, *startn = 0;

	for (p = (unsigned char*) haystack; *p; p++) {
		if (np) {
			if (cm[*p] == cm[*np]) {
				if (!*++np)
					return (char*) (startn);
			} else
				np = 0;
		}
		if (!np && cm[*p] == cm[*((const unsigned char*) needle)]) {
			np = (const unsigned char*) needle + 1;
			if (*np == 0)
				return ((char*) p);
			startn = p;
		}
	}

	return 0;
}

#ifdef  ACL_WINDOWS
#if 0
char *strcasestr(char *haystack, char *needle)
{
	char *p, *startn = 0, *np = 0;

	for (p = haystack; *p; p++) {
		if (np) {
			if (toupper(*p) == toupper(*np)) {
				if (!*++np)
					return startn;
			} else
				np = 0;
		} else if (toupper(*p) == toupper(*needle)) {
			np = needle + 1;
			startn = p;
		}
	}

	return 0;
}
#endif
#endif  /* ACL_WINDOWS */
