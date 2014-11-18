/*++
 * NAME
 *	tls_session
 * SUMMARY
 *	TLS client and server session routines
 * SYNOPSIS
 *	#include <tls.h>
 *
 *	void	tls_session_stop(ctx, stream, timeout, failure, TLScontext)
 *	TLS_APPL_STATE *ctx;
 *	ACL_VSTREAM	*stream;
 *	int	timeout;
 *	int	failure;
 *	TLS_SESS_STATE *TLScontext;
 *
 *	ACL_VSTRING	*tls_session_passivate(session)
 *	SSL_SESSION *session;
 *
 *	SSL_SESSION *tls_session_activate(session_data, session_data_len)
 *	char	*session_data;
 *	int	session_data_len;
 * DESCRIPTION
 *	tls_session_stop() implements the tls_server_shutdown()
 *	and the tls_client_shutdown() routines.
 *
 *	tls_session_passivate() converts an SSL_SESSION object to
 *	ACL_VSTRING. The result is a null pointer in case of problems,
 *	otherwise it should be disposed of with acl_vstring_free().
 *
 *	tls_session_activate() reanimates a passivated SSL_SESSION object.
 *	The result is a null pointer in case of problems,
 *	otherwise it should be disposed of with SSL_SESSION_free().
 * LICENSE
 * .ad
 * .fi
 *	This software is free. You can do with it whatever you want.
 *	The original author kindly requests that you acknowledge
 *	the use of his software.
 * AUTHOR(S)
 *	Originally written by:
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
 *
 *	Victor Duchovni
 *	Morgan Stanley
 *--*/

#include "StdAfx.h"

#ifdef USE_TLS

/* TLS library. */

#include "tls.h"
#include "tls_private.h"

/* Application-specific. */

#define STR	acl_vstring_str

/* tls_session_stop - shut down the TLS connection and reset state */

void    tls_session_stop(TLS_APPL_STATE *unused_ctx acl_unused,
	ACL_VSTREAM *stream, int timeout, int failure, TLS_SESS_STATE *TLScontext)
{
    const char *myname = "tls_session_stop";
    int     retval;

    /*
     * Sanity check.
     */
    if (TLScontext == 0)
	acl_msg_panic("%s: stream has no active TLS context", myname);

    /*
     * Perform SSL_shutdown() twice, as the first attempt will send out the
     * shutdown alert but it will not wait for the peer's shutdown alert.
     * Therefore, when we are the first party to send the alert, we must call
     * SSL_shutdown() again. On failure we don't want to resume the session,
     * so we will not perform SSL_shutdown() and the session will be removed
     * as being bad.
     */
    if (!failure) {
	retval = tls_bio_shutdown(ACL_VSTREAM_SOCK(stream), timeout, TLScontext);
	if (retval == 0)
	    tls_bio_shutdown(ACL_VSTREAM_SOCK(stream), timeout, TLScontext);
    }
    tls_free_context(TLScontext);
    tls_stream_stop(stream);
}

/* tls_session_passivate - passivate SSL_SESSION object */

ACL_VSTRING *tls_session_passivate(SSL_SESSION *session)
{
    const char *myname = "tls_session_passivate";
    int     estimate;
    int     actual_size;
    ACL_VSTRING *session_data;
    unsigned char *ptr;

    /*
     * First, find out how much memory is needed for the passivated
     * SSL_SESSION object.
     */
    estimate = i2d_SSL_SESSION(session, (unsigned char **) 0);
    if (estimate <= 0) {
	acl_msg_warn("%s: i2d_SSL_SESSION failed: unable to cache session", myname);
	return (0);
    }

    /*
     * Passivate the SSL_SESSION object. The use of a ACL_VSTRING is slightly
     * wasteful but is convenient to combine data and length.
     */
    session_data = acl_vstring_alloc(estimate);
    ptr = (unsigned char *) STR(session_data);
    actual_size = i2d_SSL_SESSION(session, &ptr);
    if (actual_size != estimate) {
	acl_msg_warn("%s: i2d_SSL_SESSION failed: unable to cache session", myname);
	acl_vstring_free(session_data);
	return (0);
    }
    ACL_VSTRING_AT_OFFSET(session_data, actual_size);	/* XXX not public */

    return (session_data);
}

/* tls_session_activate - activate passivated session */

SSL_SESSION *tls_session_activate(const char *session_data, int session_data_len)
{
#if (OPENSSL_VERSION_NUMBER < 0x0090707fL)
#define BOGUS_CONST
#else
#define BOGUS_CONST const
#endif
    SSL_SESSION *session;
    BOGUS_CONST unsigned char *ptr;

    /*
     * Activate the SSL_SESSION object.
     */
    ptr = (BOGUS_CONST unsigned char *) session_data;
    session = d2i_SSL_SESSION((SSL_SESSION **) 0, &ptr, session_data_len);
    if (!session)
	tls_print_errors();

    return (session);
}

#endif
