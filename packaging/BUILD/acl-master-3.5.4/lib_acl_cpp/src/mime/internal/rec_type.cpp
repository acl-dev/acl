/*++
 * NAME
 *	rec_type 3
 * SUMMARY
 *	Postfix record types
 * SYNOPSIS
 *	#include <rec_type.h>
 *
 *	const char *rec_type_name(type)
 *	int	type;
 * DESCRIPTION
 *	This module and its associated include file implement the
 *	Postfix-specific record types.
 *
 *	rec_type_name() returns a printable name for the given record
 *	type.
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

#include "acl_stdafx.hpp"

#if !defined(ACL_MIME_DISABLE)

/* Global library. */

#include "rec_type.hpp"

 /*
  * Lookup table with internal record type codes and printable names.
  */
typedef struct {
    int     type;
    const char *name;
} REC_TYPE_NAME;

REC_TYPE_NAME rec_type_names[] = {
	{ REC_TYPE_EOF, "end-of-file" },	/* not Postfix-specific. */
	{ REC_TYPE_ERROR, "error" },		/* not Postfix-specific. */
	{ REC_TYPE_SIZE, "message_size" },
	{ REC_TYPE_TIME, "message_arrival_time" },
	{ REC_TYPE_CTIME, "queue_file_create_time" },
	{ REC_TYPE_FULL, "sender_fullname" },
	{ REC_TYPE_INSP, "content_inspector" },
	{ REC_TYPE_FILT, "content_filter" },
	{ REC_TYPE_FROM, "sender" },
	{ REC_TYPE_DONE, "done_recipient" },
	{ REC_TYPE_DRCP, "canceled_recipient" },
	{ REC_TYPE_RCPT, "recipient" },
	{ REC_TYPE_ORCP, "original_recipient" },
	{ REC_TYPE_WARN, "warning_message_time" },
	{ REC_TYPE_ATTR, "named_attribute" },
	{ REC_TYPE_PTR, "pointer_record" },
	{ REC_TYPE_KILL, "killed_record" },
	{ REC_TYPE_MESG, "message_content" },
	{ REC_TYPE_CONT, "unterminated_text" },
	{ REC_TYPE_NORM, "regular_text" },
	{ REC_TYPE_DTXT, "padding" },
	{ REC_TYPE_XTRA, "extracted_info" },
	{ REC_TYPE_RRTO, "return_receipt" },
	{ REC_TYPE_ERTO, "errors_to" },
	{ REC_TYPE_PRIO, "priority" },
	{ REC_TYPE_VERP, "verp_delimiters" },
	{ REC_TYPE_END, "message_end" },
	{ REC_TYPE_RDR, "redirect_to" },
	{ REC_TYPE_FLGS, "flags" },
	{ REC_TYPE_DSN_RET, "dsn_return_flags" },
	{ REC_TYPE_DSN_ENVID, "dsn_envelope_id" },
	{ REC_TYPE_DSN_ORCPT, "dsn_original_recipient" },
	{ REC_TYPE_DSN_NOTIFY, "dsn_notify_flags" },
	{ 0, 0 },
};

/* rec_type_name - map record type to printable name */

const char *rec_type_name(int type)
{
    REC_TYPE_NAME *p;

    for (p = rec_type_names; p->name != 0; p++)
	if (p->type == type)
	    return (p->name);
    return ("unknown_record_type");
}

#endif // !defined(ACL_MIME_DISABLE)
