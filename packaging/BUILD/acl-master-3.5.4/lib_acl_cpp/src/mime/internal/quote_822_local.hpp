#ifndef _QUOTE_822_H_INCLUDED_
#define _QUOTE_822_H_INCLUDED_

#if !defined(ACL_MIME_DISABLE)

/*++
 * NAME
 *	quote_822_local 3h
 * SUMMARY
 *	quote local part of mailbox
 * SYNOPSIS
 *	#include "quote_822_local.h"
 * DESCRIPTION
 * .nf
 */

/*
 * Utility library.
 */
#include "stdlib/acl_vstring.h"

/*
 * Global library.
 */
#include "quote_flags.hpp"

/*
 * External interface.
 */
extern ACL_VSTRING *quote_822_local_flags(ACL_VSTRING *, const char *, int);
extern ACL_VSTRING *unquote_822_local(ACL_VSTRING *, const char *);
#define quote_822_local(dst, src) \
	quote_822_local_flags((dst), (src), QUOTE_FLAG_8BITCLEAN)

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
