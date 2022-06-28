/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>
#include <string.h>
#include <limits.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_vstring.h"
#include "code/acl_vstring_base64.h"

#endif

/* Application-specific. */

static const unsigned char to_b64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char un_b64[] = {
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

#define UNSIG_CHAR_PTR(x) ((const unsigned char *)(x))

/* vstring_base64_encode - raw data to encoded */

ACL_VSTRING *acl_vstring_base64_encode(ACL_VSTRING *result,
	const char *in, int len)
{
	const unsigned char *cp;
	int     count, size = len * 4 /3;

	ACL_VSTRING_SPACE(result, size);

	/*
	 * Encode 3 -> 4.
	 */
	ACL_VSTRING_RESET(result);
	for (cp = UNSIG_CHAR_PTR(in), count = len;
	     count > 0; count -= 3, cp += 3) {
		ACL_VSTRING_ADDCH(result, to_b64[cp[0] >> 2]);
		if (count > 1) {
			ACL_VSTRING_ADDCH(result,
				to_b64[(cp[0] & 0x3) << 4 | cp[1] >> 4]);
			if (count > 2) {
				ACL_VSTRING_ADDCH(result,
					to_b64[(cp[1] & 0xf) << 2 | cp[2] >> 6]);
				ACL_VSTRING_ADDCH(result, to_b64[cp[2] & 0x3f]);
			} else {
				ACL_VSTRING_ADDCH(result, to_b64[(cp[1] & 0xf) << 2]);
				ACL_VSTRING_ADDCH(result, '=');
				break;
			}
		} else {
			ACL_VSTRING_ADDCH(result, to_b64[(cp[0] & 0x3) << 4]);
			ACL_VSTRING_ADDCH(result, '=');
			ACL_VSTRING_ADDCH(result, '=');
			break;
		}
	}
	ACL_VSTRING_TERMINATE(result);
	return (result);
}

/* acl_vstring_base64_decode - encoded data to raw */

ACL_VSTRING *acl_vstring_base64_decode(ACL_VSTRING *result,
	const char *in, int len)
{
	const unsigned char *cp;
	int     count;
	int     ch0;
	int     ch1;
	int     ch2;
	int     ch3;

#define CHARS_PER_BYTE	(UCHAR_MAX + 1)
#define INVALID		0xff

	/*
	 * Sanity check.
	 */
	if (len % 4)
		return (NULL);

	/*
	 * Once: initialize the decoding lookup table on the fly.
	 *
	 * if (un_b64 == 0) {
	 *	un_b64 = (unsigned char *) mymalloc(CHARS_PER_BYTE);
	 *	memset(un_b64, INVALID, CHARS_PER_BYTE);
	 *	for (cp = to_b64; cp < to_b64 + sizeof(to_b64); cp++)
	 *		un_b64[*cp] = cp - to_b64;
	 * }
	 */

	ACL_VSTRING_SPACE(result, len);
	/*
	 * Decode 4 -> 3.
	 */
	ACL_VSTRING_RESET(result);

	for (cp = UNSIG_CHAR_PTR(in), count = 0; count < len; count += 4) {
		if ((ch0 = un_b64[*cp++]) == INVALID
		    || (ch1 = un_b64[*cp++]) == INVALID)
			return (0);
		ACL_VSTRING_ADDCH(result, ch0 << 2 | ch1 >> 4);
		if ((ch2 = *cp++) == '=')
			break;
		if ((ch2 = un_b64[ch2]) == INVALID)
			return (0);
		ACL_VSTRING_ADDCH(result, ch1 << 4 | ch2 >> 2);
		if ((ch3 = *cp++) == '=')
			break;
		if ((ch3 = un_b64[ch3]) == INVALID)
			return (0);
		ACL_VSTRING_ADDCH(result, ch2 << 6 | ch3);
	}
	ACL_VSTRING_TERMINATE(result);
	return (result);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to base 64 and back.
  */

#define STR(x)	vstring_str(x)
#define LEN(x)	VSTRING_LEN(x)

int     main(int unused_argc, char **unused_argv)
{
	ACL_VSTRING *b1 = acl_vstring_alloc(1);
	ACL_VSTRING *b2 = acl_vstring_alloc(1);
	char   *test = "this is a test";

#define DECODE(b,x,l) { \
	if (acl_vstring_base64_decode((b),(x),(l)) == 0) \
	    acl_msg_panic("bad base64: %s", (x)); \
    }
#define VERIFY(b,t) { \
	if (strcmp((b), (t)) != 0) \
	    acl_msg_panic("bad test: %s", (b)); \
    }

	acl_vstring_base64_encode(b1, test, strlen(test));
	DECODE(b2, STR(b1), LEN(b1));
	VERIFY(STR(b2), test);

	acl_vstring_base64_encode(b1, test, strlen(test));
	acl_vstring_base64_encode(b2, STR(b1), LEN(b1));
	acl_vstring_base64_encode(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	DECODE(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	VERIFY(STR(b2), test);

	acl_vstring_base64_encode(b1, test, strlen(test));
	acl_vstring_base64_encode(b2, STR(b1), LEN(b1));
	acl_vstring_base64_encode(b1, STR(b2), LEN(b2));
	acl_vstring_base64_encode(b2, STR(b1), LEN(b1));
	acl_vstring_base64_encode(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	DECODE(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	DECODE(b1, STR(b2), LEN(b2));
	DECODE(b2, STR(b1), LEN(b1));
	VERIFY(STR(b2), test);

	acl_vstring_free(b1);
	acl_vstring_free(b2);
	return (0);
}

#endif
