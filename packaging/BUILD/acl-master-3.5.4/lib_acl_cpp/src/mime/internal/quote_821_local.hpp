/*++
 * NAME
 *	quote_821_local 3h
 * SUMMARY
 *	quote rfc 821 local part
 * SYNOPSIS
 *	#include "quote_821_local.h"
 * DESCRIPTION
 * .nf
 */

#if !defined(ACL_MIME_DISABLE)

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
extern ACL_VSTRING *quote_821_local_flags(ACL_VSTRING *, const char *, int);
#define quote_821_local(dst, src) \
	quote_821_local_flags((dst), (src), QUOTE_FLAG_8BITCLEAN)

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
