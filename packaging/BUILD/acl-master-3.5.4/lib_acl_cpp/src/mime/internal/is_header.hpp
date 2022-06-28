#ifndef _IS_HEADER_H_INCLUDED_
#define _IS_HEADER_H_INCLUDED_

#if !defined(ACL_MIME_DISABLE)

/*++
 * NAME
 *	is_header 3h
 * SUMMARY
 *	message header classification
 * SYNOPSIS
 *	#include <is_header.h>
 * DESCRIPTION
 * .nf
 */

 /* External interface. */

#define IS_HEADER_NULL_TERMINATED	(-1)
#define is_header(str)	is_header_buf(str, IS_HEADER_NULL_TERMINATED)

extern ssize_t is_header_buf(const char *, ssize_t);

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
