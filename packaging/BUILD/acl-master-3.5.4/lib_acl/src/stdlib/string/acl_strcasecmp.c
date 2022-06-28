#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mystring.h"

#endif

#include "../charmap.h"

int acl_strcasecmp(const char *s1, const char *s2)
{
	const unsigned char *cm = maptolower,
	      *us1 = (const unsigned char *) s1,
	      *us2 = (const unsigned char *) s2;

	while (cm[*us1] == cm[*us2]) {
		if (*us1 == '\0')
			return (0);
		us1++;
		us2++;
	}
	return (cm[*us1] - cm[*us2]);
}

int acl_strncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n != 0) {
		const unsigned char *cm = maptolower,
		      *us1 = (const unsigned char *) s1,
		      *us2 = (const unsigned char *) s2;

		do {
			if (cm[*us1] != cm[*us2])
				return (cm[*us1] - cm[*us2]);
			if (*us1 == '\0')
				break;
			us1++;
			us2++;
		} while (--n != 0);
	}
	return (0);
}

int acl_strrncasecmp(const char *s1, const char *s2, size_t n)
{
	if (n != 0) {
		const unsigned char *cm = maptolower, *us1, *us2;

		if (*s1 == 0) {
			if (*s2 == 0)
				return (0);
			return (-(*s2));
		}
		if (*s2 == 0) {
			if (*s1 == 0)
				return (0);
			return (-(*s1));
		}

		us1 = (const unsigned char *) s1 + strlen(s1) - 1;
		us2 = (const unsigned char *) s2 + strlen(s2) - 1;

		do {
			if (cm[*us1] != cm[*us2])
				return (cm[*us1] - cm[*us2]);
			if (us1 == (const unsigned char*) s1) {
				if (us2 == (const unsigned char*) s2 || n == 1)
					break;
				return (-cm[*--us2]);
			}
			if (us2 == (const unsigned char*) s2) {
				if (us1 == (const unsigned char*) s1 || n == 1)
					break;
				return (-cm[*--us1]);
			}
			us1--;
			us2--;
		} while (--n != 0);
	}
	return (0);
}

int acl_strrncmp(const char *s1, const char *s2, size_t n)
{
	if (n != 0) {
		const unsigned char *us1, *us2;

		if (*s1 == 0) {
			if (*s2 == 0)
				return (0);
			return (-(*s2));
		}
		if (*s2 == 0) {
			if (*s1 == 0)
				return (0);
			return (-(*s1));
		}

		us1 = (const unsigned char *) s1 + strlen(s1) - 1;
		us2 = (const unsigned char *) s2 + strlen(s2) - 1;

		do {
			if (*us1 != *us2)
				return (*us1 - *us2);
			if (us1 == (const unsigned char*) s1) {
				if (us2 == (const unsigned char*) s2 || n == 1)
					break;
				return (-(*--us2));
			}
			if (us2 == (const unsigned char*) s2) {
				if (us1 == (const unsigned char*) s1 || n == 1)
					break;
				return (-(*--us1));
			}
			us1--;
			us2--;
		} while (--n != 0);
	}
	return (0);
}
