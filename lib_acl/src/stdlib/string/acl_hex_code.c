/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <ctype.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_hex_code.h"

#endif

/* Application-specific. */

static const unsigned char acl_hex_chars[] = "0123456789ABCDEF";

#define UCHAR_PTR(x) ((const unsigned char *)(x))

/* acl_hex_encode - raw data to encoded */

ACL_VSTRING *acl_hex_encode(ACL_VSTRING *result, const char *in, int len)
{
	const unsigned char *cp;
	int     ch;
	int     count;

	ACL_VSTRING_RESET(result);
	for (cp = UCHAR_PTR(in), count = len; count > 0; count--, cp++) {
		ch = *cp;
		ACL_VSTRING_ADDCH(result, acl_hex_chars[(ch >> 4) & 0xf]);
		ACL_VSTRING_ADDCH(result, acl_hex_chars[ch & 0xf]);
	}
	ACL_VSTRING_TERMINATE(result);
	return (result);
}

/* acl_hex_decode - encoded data to raw */

ACL_VSTRING *acl_hex_decode(ACL_VSTRING *result, const char *in, int len)
{
	const unsigned char *cp;
	int     count;
	unsigned int hex;
	unsigned int bin;

	ACL_VSTRING_RESET(result);
	for (cp = UCHAR_PTR(in), count = len; count > 0; cp += 2, count -= 2) {
		if (count < 2)
			return (0);
		hex = cp[0];
		if (hex >= '0' && hex <= '9')
			bin = (hex - '0') << 4;
		else if (hex >= 'A' && hex <= 'F')
			bin = (hex - 'A' + 10) << 4;
		else if (hex >= 'a' && hex <= 'f')
			bin = (hex - 'a' + 10) << 4;
		else
			return (0);
		hex = cp[1];
		if (hex >= '0' && hex <= '9')
			bin |= (hex - '0') ;
		else if (hex >= 'A' && hex <= 'F')
			bin |= (hex - 'A' + 10) ;
		else if (hex >= 'a' && hex <= 'f')
			bin |= (hex - 'a' + 10) ;
		else
			return (0);
		ACL_VSTRING_ADDCH(result, bin);
	}
	ACL_VSTRING_TERMINATE(result);
	return (result);
}

#ifdef TEST

 /*
  * Proof-of-concept test program: convert to hexadecimal and back.
  */

#define STR(x)	vstring_str(x)
#define LEN(x)	ACL_VSTRING_LEN(x)

int     main(int unused_argc, char **unused_argv)
{
    ACL_VSTRING *b1 = vstring_alloc(1);
    ACL_VSTRING *b2 = vstring_alloc(1);
    char   *test = "this is a test";

#define DECODE(b,x,l) { \
	if (acl_hex_decode((b),(x),(l)) == 0) \
	    acl_msg_panic("bad hex: %s", (x)); \
    }
#define VERIFY(b,t) { \
	if (strcmp((b), (t)) != 0) \
	    acl_msg_panic("bad test: %s", (b)); \
    }

    acl_hex_encode(b1, test, strlen(test));
    DECODE(b2, STR(b1), LEN(b1));
    VERIFY(STR(b2), test);

    acl_hex_encode(b1, test, strlen(test));
    acl_hex_encode(b2, STR(b1), LEN(b1));
    acl_hex_encode(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    DECODE(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    VERIFY(STR(b2), test);

    acl_hex_encode(b1, test, strlen(test));
    acl_hex_encode(b2, STR(b1), LEN(b1));
    acl_hex_encode(b1, STR(b2), LEN(b2));
    acl_hex_encode(b2, STR(b1), LEN(b1));
    acl_hex_encode(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    DECODE(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    DECODE(b1, STR(b2), LEN(b2));
    DECODE(b2, STR(b1), LEN(b1));
    VERIFY(STR(b2), test);

    vstring_free(b1);
    vstring_free(b2);
    return (0);
}

#endif

