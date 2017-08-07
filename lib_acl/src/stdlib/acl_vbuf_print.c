#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

/* System library. */

#include "stdlib/acl_define.h"

#include <stdlib.h>			/* 44BSD stdarg.h uses abort() */
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>			/* 44bsd stdarg.h uses abort() */
#include <stdio.h>			/* sprintf() prototype */
#include <float.h>			/* range of doubles */
#include <limits.h>			/* CHAR_BIT */

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Application-specific. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vbuf.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_vsprintf.h"
#include "stdlib/acl_vbuf_print.h"

#endif

 /*
  * What we need here is a *sprintf() routine that can ask for more room (as
  * in 4.4 BSD). However, that functionality is not widely available, and I
  * have no plans to maintain a complete 4.4 BSD *sprintf() alternative.
  * 
  * This means we're stuck with plain old ugly sprintf() for all non-trivial
  * conversions. We cannot use snprintf() even if it is available, because
  * that routine truncates output, and we want everything. Therefore, it is
  * up to us to ensure that sprintf() output always stays within bounds.
  * 
  * Due to the complexity of *printf() format strings we cannot easily predict
  * how long results will be without actually doing the conversions. A trick
  * used by some people is to print to a temporary file and to read the
  * result back. In programs that do a lot of formatting, that might be too
  * expensive.
  * 
  * Guessing the output size of a string (%s) conversion is not hard. The
  * problem is with numerical results. Instead of making an accurate guess we
  * take a wide margin when reserving space.  The INT_SPACE margin should be
  * large enough to hold the result from any (octal, hex, decimal) integer
  * conversion that has no explicit width or precision specifiers. With
  * floating-point numbers, use a similar estimate, and add DBL_MAX_10_EXP
  * just to be sure.
  */

#define INT_SPACE	((CHAR_BIT * sizeof(acl_int64)) / 2)
#define SIZE_T_SPACE	((CHAR_BIT * sizeof(size_t)) / 2)
#define DBL_SPACE	((CHAR_BIT * sizeof(double)) / 2 + DBL_MAX_10_EXP)
#define PTR_SPACE	((CHAR_BIT * sizeof(char *)) / 2)

 /*
  * Helper macros... Note that there is no need to check the result from
  * VSTRING_SPACE() because that always succeeds or never returns.
  */
#define VBUF_SKIP(bp)	{ \
	while ((bp)->cnt > 0 && *(bp)->ptr) \
	    (bp)->ptr++, (bp)->cnt--; \
    }

#define VBUF_STRCAT(bp, s) { \
	const unsigned char *_cp = (const unsigned char *) (s); \
	int _ch; \
	while ((_ch = *_cp++) != 0) \
	    ACL_VBUF_PUT((bp), _ch); \
    }

/* vbuf_print - format string, vsprintf-like interface */

ACL_VBUF *acl_vbuf_print(ACL_VBUF *bp, const char *format, va_list ap)
{
	static const char* null = "(null)";
	const unsigned char *cp;
	unsigned width;			/* field width */
	unsigned prec;			/* numerical precision */
	unsigned long_flag;		/* long or plain integer */
	int     ch;
	char   *s;
#define MAX_LEN	128
	char fmt[MAX_LEN + 1];		/* format specifier */
	int  i;

#define	CHECK_OVERFLOW(_i, _max) do {  \
	if (_i >= _max)  \
		acl_msg_fatal("fmt overflow: i: %d MAX: %d", (_i), (_max));  \
} while (0)

	/*
	 * Iterate over characters in the format string, picking up arguments
	 * when format specifiers are found.
	 */
	for (cp = (const unsigned char *) format; *cp; cp++) {
		if (*cp != '%') {
			ACL_VBUF_PUT(bp, *cp);		/* ordinary character */
			continue;
		} else if (cp[1] == '%') {
			ACL_VBUF_PUT(bp, *cp++);	/* %% becomes % */
			continue;
		}

		/*
		 * Handle format specifiers one at a time, since we can only
		 * deal with arguments one at a time. Try to determine the end
		 * of the format specifier. We do not attempt to fully parse
		 * format strings, since we are ging to let sprintf() do the
		 * hard work.
		 * In regular expression notation, we recognize:
		 * 
		 * %-?0?([0-9]+|\*)?\.?([0-9]+|\*)?l?[a-zA-Z]
		 * 
		 * which includes some combinations that do not make sense.
		 * Garbage in, garbage out.
		 */
		i = 0;				/* clear format string */
		fmt[i++] = *cp++;
		if (*cp == '-')			/* left-adjusted field? */
			fmt[i++] = *cp++;
		if (*cp == '+')			/* signed field? */
			fmt[i++] = *cp++;
		if (*cp == '0')			/* zero-padded field? */
			fmt[i++] = *cp++;
		if (*cp == '*') {		/* dynamic field width */
			width = va_arg(ap, int);
			sprintf(fmt + i, "%d", width);
			i = (int) strlen(fmt);	/* reset i to string length */
			cp++;
		} else {			/* hard-coded field width */
			for (width = 0; ACL_ISDIGIT(ch = *cp); cp++) {
				width = width * 10 + ch - '0';
				fmt[i++] = ch; 
				CHECK_OVERFLOW(i, MAX_LEN);
			}
		}
		if (*cp == '.')			/* width/precision separator */
			fmt[i++] = *cp++;
		if (*cp == '*') {		/* dynamic precision */
			prec = va_arg(ap, int);
			sprintf(fmt + i, "%d", prec);
			i = (int) strlen(fmt);	/* reset i to string length */
			cp++;
		} else {			/* hard-coded precision */
			for (prec = 0; ACL_ISDIGIT(ch = *cp); cp++) {
				prec = prec * 10 + ch - '0';
				fmt[i++] = ch;
				CHECK_OVERFLOW(i, MAX_LEN);
			}
		}
#ifdef ACL_WINDOWS
		if (*cp == 'l') {
			if (*(cp + 1) == 'l') {
				fmt[i++] = 'I';
				fmt[i++] = '6';
				fmt[i++] = '4';
				cp += 2;
				long_flag = 2;
			} else {
				fmt[i++] = *cp++;
				long_flag = 1;
			}
		} else if (*cp == 'z') {
			fmt[i++] = 'I';
			cp++;
			long_flag = 1;
		} else
			long_flag = 0;
#else
		if (*cp == 'l') {		/* long whatever */
			if (*(cp + 1) == 'l') {
				fmt[i++] = *cp++;
				fmt[i++] = *cp++;
				long_flag = 2;
			} else {
				fmt[i++] = *cp++;
				long_flag = 1;
				fmt[i] = 0;
			}
		} else if (*cp == 'z') {
			fmt[i++] = *cp++;
			long_flag = 1;
		} else
			long_flag = 0;
#endif
		if (*cp == 0)			/* premature end, punt */
			break;
		fmt[i++] = *cp;			/* type (checked below) */
		fmt[i] = 0;			/* null terminate */

		/*
		 * Execute the format string - let sprintf() do the hard work
		 * for non-trivial cases only. For simple string conversions
		 * and for long string conversions, do a direct copy to the
		 * output buffer.
		 */
		switch (*cp) {
		case 's':			/* string-valued argument */
			s = va_arg(ap, char *);
			if (prec > 0 || (width > 0 &&
				width > (s ? strlen(s) : strlen(null)))) {

				if (ACL_VBUF_SPACE(bp, (width > prec
					? width : prec) + INT_SPACE)) {
					return bp;
				}

				sprintf((char *) bp->ptr, fmt, s ? s : null);
				VBUF_SKIP(bp);
			} else {
				VBUF_STRCAT(bp, s ? s : null);
			}
			break;
		case 'c':			/* integral-valued argument */
		case 'd':
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			if (ACL_VBUF_SPACE(bp, (width > prec
				? width : prec) + INT_SPACE)) {

				return bp;
			}
			if (long_flag == 0)
				sprintf((char *) bp->ptr, fmt, va_arg(ap, int));
			else if (long_flag == 1)
				sprintf((char *) bp->ptr, fmt, va_arg(ap, long));
			else if (long_flag == 2)
				sprintf((char *) bp->ptr, fmt, va_arg(ap, acl_int64));
			else
				acl_msg_panic("%s: unknown format type: %c,"
					" long_flag: %d", __FUNCTION__,
					*cp, long_flag);
			VBUF_SKIP(bp);
			break;
		case 'e':			/* float-valued argument */
		case 'f':
		case 'g':
			if (ACL_VBUF_SPACE(bp, (width > prec
				? width : prec) + DBL_SPACE)) {

				return bp;
			}
			sprintf((char *) bp->ptr, fmt, va_arg(ap, double));
			VBUF_SKIP(bp);
			break;
		case 'm':
			VBUF_STRCAT(bp, acl_last_serror());
			break;
		case 'p':
			if (ACL_VBUF_SPACE(bp, (width > prec
				? width : prec) + PTR_SPACE)) {

				return bp;
			}
			sprintf((char *) bp->ptr, fmt, va_arg(ap, char *));
			VBUF_SKIP(bp);
			break;
		default:			/* anything else is bad */
			acl_msg_panic("%s: unknown format type: %c",
				__FUNCTION__, *cp);
			/* NOTREACHED */
			break;
		}
	}

	return bp;
}
