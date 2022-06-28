/*++
 * NAME
 *	quote_822_local 3
 * SUMMARY
 *	quote local part of mailbox
 * SYNOPSIS
 *	#include <quote_822_local.h>
 *
 *	VSTRING	*quote_822_local(dst, src)
 *	VSTRING	*dst;
 *	const char *src;
 *
 *	VSTRING	*quote_822_local_flags(dst, src, flags)
 *	VSTRING	*dst;
 *	const char *src;
 *	int	flags;
 *
 *	VSTRING	*unquote_822_local(dst, src)
 *	VSTRING	*dst;
 *	const char *src;
 * DESCRIPTION
 *	quote_822_local() quotes the local part of a mailbox and
 *	returns a result that can be used in message headers as
 *	specified by RFC 822 (actually, an 8-bit clean version of
 *	RFC 822). It implements an 8-bit clean version of RFC 822.
 *
 *	quote_822_local_flags() provides finer control.
 *
 *	unquote_822_local() transforms the local part of a mailbox
 *	address to unquoted (internal) form.
 *
 *	Arguments:
 * .IP dst
 *	The result.
 * .IP src
 *	The input address.
 * .IP flags
 *	Bit-wise OR of zero or more of the following.
 * .RS
 * .IP QUOTE_FLAG_8BITCLEAN
 *	In violation with RFCs, treat 8-bit text as ordinary text.
 * .IP QUOTE_FLAG_EXPOSE_AT
 *	In violation with RFCs, treat `@' as an ordinary character.
 * .IP QUOTE_FLAG_APPEND
 *	Append to the result buffer, instead of overwriting it.
 * .RE
 * STANDARDS
 *	RFC 822 (ARPA Internet Text Messages)
 * BUGS
 *	The code assumes that the domain is RFC 822 clean.
 * LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

/* System library. */

#include "acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

#include <string.h>
#include <ctype.h>

/* Utility library. */

#include "stdlib/acl_vstring.h"

/* Global library. */

/* Application-specific. */

#include "quote_822_local.hpp"

/* Local stuff. */

#define YES	1
#define	NO	0

/* is_822_dot_string - is this local-part an rfc 822 dot-string? */

static int is_822_dot_string(const char *local_part, const char *end, int flags)
{
	const char *cp;
	int     ch;

	/*
	 * Detect any deviations from a sequence of atoms separated by dots. We
	 * could use lookup tables to speed up some of the work, but hey, how
	 * large can a local-part be anyway?
	 * 
	 * RFC 822 expects 7-bit data. Rather than quoting every 8-bit character
	 * (and still passing it on as 8-bit data) we leave 8-bit data alone.
	 */
	if (local_part == end || local_part[0] == 0 || local_part[0] == '.')
		return (NO);
	for (cp = local_part; cp < end && (ch = *(unsigned char *) cp) != 0; cp++) {
		if (ch == '.' && (cp + 1) < end && cp[1] == '.')
			return (NO);
		if (ch > 127 && !(flags & QUOTE_FLAG_8BITCLEAN))
			return (NO);
		if (ch == ' ')
			return (NO);
		if (ACL_ISCNTRL(ch))
			return (NO);
		if (ch == '(' || ch == ')'
			|| ch == '<' || ch == '>'
			|| (ch == '@' && !(flags & QUOTE_FLAG_EXPOSE_AT)) || ch == ','
			|| ch == ';' || ch == ':'
			|| ch == '\\' || ch == '"'
			|| ch == '[' || ch == ']')
		{
			return (NO);
		}
	}
	if (cp[-1] == '.')
		return (NO);
	return (YES);
}

/* make_822_quoted_string - make quoted-string from local-part */

static ACL_VSTRING *make_822_quoted_string(ACL_VSTRING *dst, const char *local_part,
		const char *end, int flags)
{
	const char *cp;
	int     ch;

	/*
	 * Put quotes around the result, and prepend a backslash to characters
	 * that need quoting when they occur in a quoted-string.
	 */
	ACL_VSTRING_ADDCH(dst, '"');
	for (cp = local_part; cp < end && (ch = *(const unsigned char *) cp) != 0; cp++) {
		if ((ch > 127 && !(flags & QUOTE_FLAG_8BITCLEAN))
			|| ch == '"' || ch == '\\' || ch == '\r')
		{
			ACL_VSTRING_ADDCH(dst, '\\');
		}
		ACL_VSTRING_ADDCH(dst, ch);
	}
	ACL_VSTRING_ADDCH(dst, '"');
	return (dst);
}

/* quote_822_local_flags - quote local part of mailbox according to rfc 822 */

ACL_VSTRING *quote_822_local_flags(ACL_VSTRING *dst, const char *mbox, int flags)
{
	const char *start;			/* first byte of localpart */
	const char *end;			/* first byte after localpart */
	const char *colon;

	/*
	 * According to RFC 822, a local-part is a dot-string or a quoted-string.
	 * We first see if the local-part is a dot-string. If it is not, we turn
	 * it into a quoted-string. Anything else would be too painful. But
	 * first, skip over any source route that precedes the local-part.
	 */
	if (mbox[0] == '@' && (colon = strchr(mbox, ':')) != 0)
		start = colon + 1;
	else
		start = mbox;
	if ((end = strrchr(start, '@')) == 0)
		end = start + strlen(start);
	if ((flags & QUOTE_FLAG_APPEND) == 0)
		ACL_VSTRING_RESET(dst);
	if (is_822_dot_string(start, end, flags)) {
		return (acl_vstring_strcat(dst, mbox));
	} else {
		acl_vstring_strncat(dst, mbox, start - mbox);
		make_822_quoted_string(dst, start, end, flags & QUOTE_FLAG_8BITCLEAN);
		return (acl_vstring_strcat(dst, end));
	}
}

/* unquote_822_local - unquote local part of mailbox according to rfc 822 */

ACL_VSTRING *unquote_822_local(ACL_VSTRING *dst, const char *mbox)
{
	const char *start;			/* first byte of localpart */
	const char *end;			/* first byte after localpart */
	const char *colon;
	const char *cp;

	if (mbox[0] == '@' && (colon = strchr(mbox, ':')) != 0) {
		start = colon + 1;
		acl_vstring_strncpy(dst, mbox, start - mbox);
	} else {
		start = mbox;
		ACL_VSTRING_RESET(dst);
	}
	if ((end = strrchr(start, '@')) == 0)
		end = start + strlen(start);
	for (cp = start; cp < end; cp++) {
		if (*cp == '"')
			continue;
		if (*cp == '\\') {
			if (cp[1] == 0)
				continue;
			cp++;
		}
		ACL_VSTRING_ADDCH(dst, *cp);
	}
	if (*end)
		acl_vstring_strcat(dst, end);
	else
		ACL_VSTRING_TERMINATE(dst);
	return (dst);
}

#ifdef TEST

/*
 * Proof-of-concept test program. Read an unquoted address from stdin, and
 * show the quoted and unquoted results.
 */
#include <vstream.h>
#include <vstring_vstream.h>

#define STR	vstring_str

int     main(int unused_argc, char **unused_argv)
{
	VSTRING *raw = vstring_alloc(100);
	VSTRING *quoted = vstring_alloc(100);
	VSTRING *unquoted = vstring_alloc(100);

	while (vstring_fgets_nonl(raw, VSTREAM_IN)) {
		quote_822_local(quoted, STR(raw));
		vstream_printf("quoted:		%s\n", STR(quoted));
		unquote_822_local(unquoted, STR(quoted));
		vstream_printf("unquoted:	%s\n", STR(unquoted));
		vstream_fflush(VSTREAM_OUT);
	}
	vstring_free(unquoted);
	vstring_free(quoted);
	vstring_free(raw);
	return (0);
}

#endif
#endif // !defined(ACL_MIME_DISABLE)
