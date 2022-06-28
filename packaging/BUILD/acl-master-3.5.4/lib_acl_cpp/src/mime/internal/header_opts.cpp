/*++
* NAME
*	header_opts 3
* SUMMARY
*	message header classification
* SYNOPSIS
*	#include <header_opts.h>
*
*	const HEADER_OPTS *header_opts_find(string)
*	const char *string;
* DESCRIPTION
*	header_opts_find() takes a message header line and looks up control
*	information for the corresponding header type.
* DIAGNOSTICS
*	Panic: input is not a valid header line. The result is a pointer
*	to HEADER_OPTS in case of success, a null pointer when the header
*	label was not recognized.
* SEE ALSO
*	header_opts(3h) the gory details
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

#include <ctype.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_stringops.h"
#include "thread/acl_pthread.h"
/* Global library. */

#include "trimblanks.hpp"
#include "header_opts.hpp"

/*
* Header names are given in the preferred capitalization. The lookups are
* case-insensitive.
* 
* XXX Removing Return-Path: headers should probably be done only with mail
* that enters via a non-SMTP channel. Changing this now could break other
* software. See also comments in bounce_notify_util.c.
*/
static const HEADER_OPTS header_opts[] = {
	{ "Apparently-To", HDR_APPARENTLY_TO, HDR_OPT_RECIP },
	{ "Bcc", HDR_BCC, HDR_OPT_DROP | HDR_OPT_XRECIP },
	{ "Cc", HDR_CC, HDR_OPT_XRECIP },
	{ "Content-Description", HDR_CONTENT_DESCRIPTION, HDR_OPT_MIME },
	{ "Content-Disposition", HDR_CONTENT_DISPOSITION, HDR_OPT_MIME },
	{ "Content-ID", HDR_CONTENT_ID, HDR_OPT_MIME },
	{ "Content-Length", HDR_CONTENT_LENGTH, HDR_OPT_DROP },
	{ "Content-Transfer-Encoding", HDR_CONTENT_TRANSFER_ENCODING, HDR_OPT_MIME },
	{ "Content-Type", HDR_CONTENT_TYPE, HDR_OPT_MIME },
	{ "Delivered-To", HDR_DELIVERED_TO, 0 },
	{ "Disposition-Notification-To", HDR_DISP_NOTIFICATION, HDR_OPT_SENDER },
	{ "Date", HDR_DATE, 0 },
	{ "Errors-To", HDR_ERRORS_TO, HDR_OPT_SENDER },
	{ "From", HDR_FROM, HDR_OPT_SENDER },
	{ "Mail-Followup-To", HDR_MAIL_FOLLOWUP_TO, HDR_OPT_SENDER },
	{ "Message-Id", HDR_MESSAGE_ID, 0 },
	{ "MIME-Version", HDR_MIME_VERSION, HDR_OPT_MIME },
	{ "Received", HDR_RECEIVED, 0 },
	{ "Reply-To", HDR_REPLY_TO, HDR_OPT_SENDER },
	{ "Resent-Bcc", HDR_RESENT_BCC, HDR_OPT_DROP | HDR_OPT_XRECIP | HDR_OPT_RR },
	{ "Resent-Cc", HDR_RESENT_CC, HDR_OPT_XRECIP | HDR_OPT_RR },
	{ "Resent-Date", HDR_RESENT_DATE, HDR_OPT_RR },
	{ "Resent-From", HDR_RESENT_FROM, HDR_OPT_SENDER | HDR_OPT_RR },
	{ "Resent-Message-Id", HDR_RESENT_MESSAGE_ID, HDR_OPT_RR },
	{ "Resent-Reply-To", HDR_RESENT_REPLY_TO, HDR_OPT_RECIP | HDR_OPT_RR },
	{ "Resent-Sender", HDR_RESENT_SENDER, HDR_OPT_SENDER | HDR_OPT_RR },
	{ "Resent-To", HDR_RESENT_TO, HDR_OPT_XRECIP | HDR_OPT_RR },
	{ "Return-Path", HDR_RETURN_PATH, HDR_OPT_DROP | HDR_OPT_SENDER },
	{ "Return-Receipt-To", HDR_RETURN_RECEIPT_TO, HDR_OPT_SENDER },
	{ "Sender", HDR_SENDER, HDR_OPT_SENDER },
	{ "To", HDR_TO, HDR_OPT_XRECIP },
	{ "Subject", HDR_SUBJECT, HDR_OPT_SUBJECT },
};

#define HEADER_OPTS_SIZE (sizeof(header_opts) / sizeof(header_opts[0]))

typedef struct {
	ACL_HTABLE *header_hash;
} HEADER_CTX;

#define STR	acl_vstring_str

/* header_opts_init - initialize */

static void header_opts_init(ACL_HTABLE *header_hash)
{
	const HEADER_OPTS *hp;
	const char *cp;
	ACL_VSTRING *header_key = acl_vstring_alloc(128);

	/*
	 * Build a hash table for quick lookup, and allocate memory for
	 * lower-casing the lookup key.
	 */
	for (hp = header_opts; hp < header_opts + HEADER_OPTS_SIZE; hp++) {
		ACL_VSTRING_RESET(header_key);
		for (cp = hp->name; *cp; cp++)
			ACL_VSTRING_ADDCH(header_key, ACL_TOLOWER(*cp));
		ACL_VSTRING_TERMINATE(header_key);
		acl_htable_enter(header_hash, STR(header_key), (void*) hp);
	}

	acl_vstring_free(header_key);
}

static HEADER_CTX *header_ctx = NULL;

#ifndef HAVE_NO_ATEXIT
static void header_ctx_free(void)
{
	if (header_ctx == NULL)
		return;
	acl_htable_free(header_ctx->header_hash, NULL);
	acl_myfree(header_ctx);
	header_ctx = NULL;
}
#endif

static void header_opts_once(void)
{
	header_ctx = (HEADER_CTX*) acl_mymalloc(sizeof(HEADER_CTX));
	header_ctx->header_hash = acl_htable_create(HEADER_OPTS_SIZE, 0);
	header_opts_init(header_ctx->header_hash);

#ifndef HAVE_NO_ATEXIT
	atexit(header_ctx_free);
#endif
}

static acl_pthread_once_t __header_once = ACL_PTHREAD_ONCE_INIT;

/* header_opts_find - look up header options */

const HEADER_OPTS *header_opts_find(const char *string, ACL_VSTRING *key_buffer)
{
	const char *myname = "header_opts_find";
	const char *cp;
	HEADER_OPTS *header_info;
	ACL_VSTRING *header_key;

	acl_pthread_once(&__header_once, header_opts_once);

	if (header_ctx == NULL)
		acl_msg_fatal("%s(%d): header_ctx!", myname, __LINE__);

	if (key_buffer == NULL)
		header_key = acl_vstring_alloc(128);
	else
		header_key = key_buffer;

	/*
	 * Look up the lower-cased version of the header name.
	 */
	ACL_VSTRING_RESET(header_key);
	for (cp = string; *cp != ':'; cp++) {
		if (*cp == 0) {
			acl_msg_error("%s: no colon in header: %.30s",
					myname, string);
			if (key_buffer == NULL)
				acl_vstring_free(header_key);
			return (NULL);
		}
		ACL_VSTRING_ADDCH(header_key, ACL_TOLOWER(*cp));
	}
	acl_vstring_truncate(header_key,
		trimblanks(STR(header_key), (int) (cp - string))
			- STR(header_key));
	ACL_VSTRING_TERMINATE(header_key);

	header_info = (HEADER_OPTS*) acl_htable_find(header_ctx->header_hash,
			STR(header_key));
	if (key_buffer == NULL)
		acl_vstring_free(header_key);

	return (header_info);
}

HEADER_NV *header_split(const char *s)
{
	HEADER_NV *header;

#define	SKIP_SPACE(x) {  \
	while (*(x) == ' ' || *(x) == '\t')  \
		(x)++;  \
}
	SKIP_SPACE(s);
	if (*s == 0)
		return (NULL);

	header = (HEADER_NV*) acl_mycalloc(1, sizeof(HEADER_NV));
	header->name = acl_mystrdup(s);
	char *ptr = strchr(header->name, ':');
	if (ptr == NULL) {
		header_nv_free(header);
		return (NULL);
	}
	*ptr++ = 0;
	SKIP_SPACE(ptr);
	if (*ptr == 0) {
		header_nv_free(header);
		return (NULL);
	}
	header->value = ptr;
	return (header);
}

void header_nv_free(HEADER_NV *header)
{
	if (header) {
		acl_myfree(header->name);
		acl_myfree(header);
	}
}

#endif // !defined(ACL_MIME_DISABLE)
