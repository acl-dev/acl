#ifndef _LEX_822_H_INCLUDED_
#define _LEX_822_H_INCLUDED_

#if !defined(ACL_MIME_DISABLE)

/*++
 * NAME
 *	lex_822 3h
 * SUMMARY
 *	RFC822 lexicals
 * SYNOPSIS
 *	#include <lex_822.h>
 * DESCRIPTION
 * .nf
 */

/*
 * The predicate macros.
 */
#define IS_SPACE_TAB(ch)	(ch == ' ' || ch == '\t')
#define IS_SPACE_TAB_CR_LF(ch)	(IS_SPACE_TAB(ch) || ch == '\r' || ch == '\n')

/*
 * Special characters as per RFC 822.
 */
#define LEX_822_SPECIALS	"()<>@,;:\\\".[]"

/* LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#endif // !defined(ACL_MIME_DISABLE)
#endif
