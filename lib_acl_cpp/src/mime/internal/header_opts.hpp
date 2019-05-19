#ifndef _HEADER_OPTS_H_INCLUDED_
#define _HEADER_OPTS_H_INCLUDED_

#if !defined(ACL_MIME_DISABLE)

/*++
 * NAME
 *	header_opts 3h
 * SUMMARY
 *	message header classification
 * SYNOPSIS
 *	#include <header_opts.h>
 * DESCRIPTION
 * .nf
 */

 /* External interface. */
typedef struct MIME_STATE MIME_STATE;

struct HEADER_OPTS {
    const char *name;			/* name, preferred capitalization */
    int     type;			/* type, see below */
    int     flags;			/* flags, see below */
};

typedef struct HEADER_NV HEADER_NV;

struct HEADER_NV {
	char *name;
	char *value;
};

 /*
  * Header types. If we reach 31, we must group the headers we need to
  * remember at the beginning, or we should use fd_set bit sets.
  */
#define HDR_APPARENTLY_TO       1
#define HDR_BCC                 2
#define HDR_CC                  3
#define HDR_CONTENT_LENGTH      4
#define HDR_CONTENT_TRANSFER_ENCODING    5
#define HDR_CONTENT_TYPE        6
#define HDR_DATE                7
#define HDR_DELIVERED_TO        8
#define HDR_ERRORS_TO           9
#define HDR_FROM                10
#define HDR_MESSAGE_ID          11
#define HDR_RECEIVED            12
#define HDR_REPLY_TO            13
#define HDR_RESENT_BCC          14
#define HDR_RESENT_CC           15
#define HDR_RESENT_DATE         16
#define HDR_RESENT_FROM         17
#define HDR_RESENT_MESSAGE_ID   18
#define HDR_RESENT_REPLY_TO     19
#define HDR_RESENT_SENDER       20
#define HDR_RESENT_TO           21
#define HDR_RETURN_PATH         22
#define HDR_RETURN_RECEIPT_TO   23
#define HDR_SENDER              24
#define HDR_TO                  25
#define HDR_MAIL_FOLLOWUP_TO    26
#define HDR_CONTENT_DESCRIPTION 27
#define HDR_CONTENT_DISPOSITION 28
#define HDR_CONTENT_ID          29
#define HDR_MIME_VERSION        30
#define HDR_DISP_NOTIFICATION   31
#define	HDR_SUBJECT             32

 /*
  * Header flags.
  */
#define HDR_OPT_DROP    (1<<0)		/* delete from input */
#define HDR_OPT_SENDER  (1<<1)		/* sender address */
#define HDR_OPT_RECIP   (1<<2)		/* recipient address */
#define HDR_OPT_RR      (1<<3)		/* Resent- header */
#define HDR_OPT_EXTRACT (1<<4)		/* extract flag */
#define HDR_OPT_MIME    (1<<5)		/* MIME header */
#define	HDR_OPT_SUBJECT (1<<6)		/* subject */

#define HDR_OPT_XRECIP	(HDR_OPT_RECIP | HDR_OPT_EXTRACT)

extern const HEADER_OPTS *header_opts_find(const char *, ACL_VSTRING*);
extern HEADER_NV *header_split(const char *s);
extern void header_nv_free(HEADER_NV *header);

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
