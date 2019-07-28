/*++
 * NAME
 *	tls_stream
 * SUMMARY
 *	ACL_VSTREAM over TLS
 * SYNOPSIS
 *	#include <tls.h>
 *	#include <tls_private.h>
 *
 *	void	tls_stream_start(stream, context)
 *	ACL_VSTREAM	*stream;
 *	TLS_SESS_STATE *context;
 *
 *	void	tls_stream_stop(stream)
 *	ACL_VSTREAM	*stream;
 * DESCRIPTION
 *	This module implements the ACL_VSTREAM over TLS support user interface.
 *	The hard work is done elsewhere.
 *
 *	tls_stream_start() enables TLS on the named stream. All read
 *	and write operations are directed through the TLS library,
 *	using the state information specified with the context argument.
 *
 *	tls_stream_stop() replaces the ACL_VSTREAM read/write routines
 *	by dummies that have no side effects, and deletes the
 *	ACL_VSTREAM's reference to the TLS context.
 * SEE ALSO
 *	dummy_read(3), placebo read routine
 *	dummy_write(3), placebo write routine
 * LICENSE
 * .ad
 * .fi
 *	This software is free. You can do with it whatever you want.
 *	The original author kindly requests that you acknowledge
 *	the use of his software.
 * AUTHOR(S)
 *	Based on code that was originally written by:
 *	Lutz Jaenicke
 *	BTU Cottbus
 *	Allgemeine Elektrotechnik
 *	Universitaetsplatz 3-4
 *	D-03044 Cottbus, Germany
 *
 *	Updated by:
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#include "StdAfx.h"

#ifdef USE_TLS

/* TLS library. */

#include "tls.h"
#include "tls_private.h"

/* tls_timed_read - read content from stream, then TLS decapsulate */

static ssize_t tls_timed_read(int fd, void *buf, size_t len,
	int timeout, void *context)
{
    const char *myname = "tls_timed_read";
    ssize_t ret;
    TLS_SESS_STATE *TLScontext;

    TLScontext = (TLS_SESS_STATE *) context;
    if (!TLScontext)
	acl_msg_panic("%s: no context", myname);

    ret = tls_bio_read(fd, buf, (int) len, timeout, TLScontext);
    if (ret > 0 && TLScontext->log_level >= 4)
	acl_msg_info("Read %ld chars: %.*s",
		 (long) ret, (int) (ret > 40 ? 40 : ret), (char *) buf);
    return (ret);
}

/* tls_timed_write - TLS encapsulate content, then write to stream */

static ssize_t tls_timed_write(int fd, void *buf, size_t len,
	int timeout, void *context)
{
    const char *myname = "tls_timed_write";
    TLS_SESS_STATE *TLScontext;

    TLScontext = (TLS_SESS_STATE *) context;
    if (!TLScontext)
	acl_msg_panic("%s: no context, buf(%s), len(%d)", myname, (char*) buf, (int) len);

    if (TLScontext->log_level >= 4)
	acl_msg_info("Write %ld chars: %.*s",
		 (long) len, (int) (len > 40 ? 40 : len), (char *) buf);
    return (tls_bio_write(fd, buf, (int) len, timeout, TLScontext));
}

/* tls_stream_start - start ACL_VSTREAM over TLS */

void    tls_stream_start(ACL_VSTREAM *stream, TLS_SESS_STATE *context)
{
    acl_vstream_ctl(stream,
		    ACL_VSTREAM_CTL_READ_FN, tls_timed_read,
		    ACL_VSTREAM_CTL_WRITE_FN, tls_timed_write,
		    ACL_VSTREAM_CTL_CTX, (void *) context,
		    ACL_VSTREAM_CTL_END);
}

static ssize_t dummy_read(int fd acl_unused, void *buf acl_unused,
	size_t len acl_unused, int timeout acl_unused, void *context acl_unused)
{
	return (ACL_VSTREAM_EOF);
}

static ssize_t dummy_write(int fd acl_unused, void *buf acl_unused,
	size_t len acl_unused, int timeout acl_unused, void *context acl_unused)
{
	return (ACL_VSTREAM_EOF);
}

/* tls_stream_stop - stop ACL_VSTREAM over TLS */

void    tls_stream_stop(ACL_VSTREAM *stream)
{

    /*
     * Prevent data leakage after TLS is turned off. The Postfix/TLS patch
     * provided null function pointers; we use dummy routines that make less
     * noise when used.
     */
    acl_vstream_ctl(stream,
		    ACL_VSTREAM_CTL_READ_FN, dummy_read,
		    ACL_VSTREAM_CTL_WRITE_FN, dummy_write,
		    ACL_VSTREAM_CTL_CONTEXT, (void *) 0,
		    ACL_VSTREAM_CTL_END);
}

#endif
