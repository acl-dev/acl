#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_mymalloc.h"
#include "code/acl_base64.h"

#endif

static const unsigned char to_b64_tab[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char un_b64_tab[] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 62,  255, 255, 255, 63,
	52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  255, 255, 255, 255, 255, 255,
	255, 0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
	15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  255, 255, 255, 255, 255,
	255, 26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,
	41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51,  255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255
};

unsigned char *acl_base64_encode(const char *in, int len)
{
	const unsigned char *clear = (const unsigned char*) in;
	unsigned char *code = NULL;
	unsigned char *p;

	code = acl_mymalloc(4 * ((len + 2) / 3) + 1);
	p = code;

	while (len-- >0) {
		register int x, y;

		x = *clear++;
		*p++ = to_b64_tab[(x >> 2) & 63];

		if (len-- <= 0) {
			*p++ = to_b64_tab[(x << 4) & 63];
			*p++ = '=';
			*p++ = '=';
			break;
		}

		y = *clear++;
		*p++ = to_b64_tab[((x << 4) | ((y >> 4) & 15)) & 63];

		if (len-- <= 0) {
			*p++ = to_b64_tab[(y << 2) & 63];
			*p++ = '=';
			break;
		}

		x = *clear++;
		*p++ = to_b64_tab[((y << 2) | ((x >> 6) & 3)) & 63];

		*p++ = to_b64_tab[x & 63];
	}

	*p = 0;

	return (code);
}

int acl_base64_decode(const char *in, char **pptr_in)
{
	unsigned int in_len;
	const unsigned char *in_end;
	const unsigned char *code = (const unsigned char*) in;
	unsigned char **pptr = (unsigned char**) pptr_in;
	register int x, y;
	unsigned char *result, *result_saved = NULL;

#undef	ERETURN
#define	ERETURN(x) { \
	if (result_saved != NULL) \
		acl_myfree(result_saved); \
	*pptr = NULL; \
	return (x); \
}

	in_len = strlen((const char *) code);
	in_end = code + in_len;
	result_saved = result = acl_mymalloc(3 * (in_len / 4) + 1);
	*pptr = result;

	/* Each cycle of the loop handles a quantum of 4 input bytes. For the last
	   quantum this may decode to 1, 2, or 3 output bytes. */

	while (in_end - code >= 4 && (x = (*code++)) != 0) {
		if (x > 127 || (x = un_b64_tab[x]) == 255)
			ERETURN (-1);
		if ((y = (*code++)) == 0 || (y = un_b64_tab[y]) == 255)
			ERETURN (-1);
		*result++ = (x << 2) | (y >> 4);

		if ((x = (*code++)) == '=') {
			if (*code++ != '=' || *code != 0)
				ERETURN (-1);
		} else {
			if (x > 127 || (x = un_b64_tab[x]) == 255)
				ERETURN (-1);
			*result++ = (y << 4) | (x >> 2);
			if ((y = (*code++)) == '=') {
				if (*code != 0)
					ERETURN (-1);
			} else {
				if (y > 127 || (y = un_b64_tab[y]) == 255)
					ERETURN (-1);
				*result++ = (x << 6) | y;
			}
		}
	}

	*result = 0;
	return (int) (result - *pptr);
}
