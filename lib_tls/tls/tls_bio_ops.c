/*++
 * NAME
 *	tls_bio_ops 3
 * SUMMARY
 *	TLS network BIO management
 * SYNOPSIS
 *	#include <tls.h>
 *	#include <tls_private.h>
 *
 *	int tls_bio_connect(fd, timeout, context)
 *	int	fd;
 *	int	timeout;
 *	TLS_SESS_STATE *context;
 *
 *	int tls_bio_accept(fd, timeout, context)
 *	int	fd;
 *	int	timeout;
 *	TLS_SESS_STATE *context;
 *
 *	int tls_bio_shutdown(fd, timeout, context)
 *	int	fd;
 *	int	timeout;
 *	TLS_SESS_STATE *context;
 *
 *	int tls_bio_read(fd, buf, len, timeout, context)
 *	int	fd;
 *	void	*buf;
 *	int	len;
 *	int	timeout;
 *	TLS_SESS_STATE *context;
 *
 *	int tls_bio_write(fd, buf, len, timeout, context)
 *	int	fd;
 *	void	*buf;
 *	int	len;
 *	int	timeout;
 *	TLS_SESS_STATE *context;
 * DESCRIPTION
 *	This layer synchronizes the TLS network buffers with the network
 *	while performing TLS handshake or input/output operations.
 *
 *	When the TLS layer is active, it converts plain-text
 *	data from Postfix into encrypted network data and vice versa.
 *	However, to handle network timeout conditions, Postfix
 *	needs to maintain control over network input/output. This
 *	rules out the usual approach of placing the TLS layer
 *	between the application and the network socket.
 *
 *	As shown below, Postfix reads/writes plain-text data from/to
 *	the TLS layer. The TLS layer informs Postfix when it needs
 *	to read/write encrypted data from/to the network; Postfix
 *	then reads/writes encrypted data from/to the TLS layer and
 *	takes care of the network socket I/O.
 *
 *	The TLS layer to network interface is realized with a BIO pair:
 *
 *	 Postfix             |   TLS layer
 *	                     |
 *	smtp/smtpd           |
 *	 /\    ||            |
 *	 ||    \/            |
 *	vstream read/write <===> TLS read/write/etc
 *	                     |     /\    ||
 *	                     |     ||    \/
 *	                     |   BIO pair (internal_bio)
 *	                     |   BIO pair (network_bio)
 *	                     |     /\    ||
 *	                     |     ||    \/
 *	socket read/write  <===> BIO read/write
 *	 /\    ||            |
 *	 ||    \/            |
 *	 network             |
 *
 *	The Postfix VSTREAM read/write operations invoke the SSL
 *	read/write operations to send and retrieve plain-text data. Inside
 *	the TLS layer the data are converted to/from TLS protocol.
 *
 *	Whenever an SSL operation reports success, or whenever it
 *	indicates that network input/output needs to happen, Postfix
 *	uses the BIO read/write routines to synchronize the
 *	network_bio buffer with the network.  Writing data to the
 *	network has precedence over reading from the network. This
 *	is necessary to avoid deadlock.
 *
 *	The BIO pair buffer size is set to 8192 bytes. This is much
 *	larger than the typical Path MTU, and avoids sending tiny TCP
 *	segments.  It is also larger than the default VSTREAM_BUFSIZE
 *	(4096, see vstream.h), so that large write operations can
 *	be handled within one request.  The internal buffer in the
 *	network/network_bio handling layer is set to the same
 *	value, since this seems to be reasonable. The code is
 *	however able to handle arbitrary values smaller or larger
 *	than the buffer size in the BIO pair.
 *
 *	tls_bio_connect() performs the SSL_connect() operation while
 *	synchronizing the network_bio buffer with the network.
 *
 *	tls_bio_accept() performs the SSL_accept() operation while
 *	synchronizing the network_bio buffer with the network.
 *
 *	tls_bio_shutdown() performs the SSL_shutdown() operation while
 *	synchronizing the network_bio buffer with the network.
 *
 *	tls_bio_read() performs the SSL_read() operation while
 *	synchronizing the network_bio buffer with the network.
 *
 *	tls_bio_write() performs the SSL_write() operation while
 *	synchronizing the network_bio buffer with the network.
 *
 *	Arguments:
 * .IP fd
 *	Network socket.
 * .IP buf
 *	Read/write buffer.
 * .IP len
 *	Read/write request size.
 * .IP timeout
 *	Read/write timeout.
 * .IP TLScontext
 *	TLS session state.
 * DIAGNOSTICS
 *	The result value is -1 in case of a network read/write
 *	error, otherwise it is the result value of the TLS operation.
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

/* System library. */

#include "StdAfx.h"

#ifdef USE_TLS

/* TLS library. */

#include "tls.h"
#include "tls_private.h"

/* Application-specific. */

#define NETLAYER_BUFFERSIZE 8192

/* network_biopair_interop - synchronize network with BIO pair */

static int network_biopair_interop(ACL_SOCKET fd, int timeout, BIO *network_bio)
{
    const char *myname = "network_biopair_interop";
    int     want_write;
    int     num_write;
    int     write_pos;
    int     from_bio;
    int     want_read;
    int     num_read;
    int     to_bio;
    char    buffer[NETLAYER_BUFFERSIZE];

    /*
     * To avoid deadlock, write all pending data to the network before
     * attempting to read from the network.
     */
    while ((want_write = (int) BIO_ctrl_pending(network_bio)) > 0) {
	if (want_write > (int) sizeof(buffer))
	    want_write = (int) sizeof(buffer);
	from_bio = BIO_read(network_bio, buffer, want_write);

	/*
	 * Write the complete buffer contents to the network.
	 */
	for (write_pos = 0; write_pos < from_bio; /* see below */ ) {
	    if (timeout > 0 && acl_write_wait(fd, timeout) < 0)
		return (-1);
	    num_write = acl_socket_write(fd, buffer + write_pos, from_bio - write_pos, 0, 0, 0);
	    if (num_write <= 0) {
		if ((num_write < 0) && (timeout > 0) && (errno == ACL_EAGAIN || errno == ACL_EINTR)) {
		    acl_msg_warn("%s: write() returns EAGAIN on a writable file descriptor!", myname);
		    acl_msg_warn("%s: pausing to avoid going into a tight select/write loop!", myname);
		    sleep(1);
		} else {
		    acl_msg_warn("%s: error writing %d bytes to the network: %s",
			    myname, from_bio - write_pos, acl_last_serror());
		    return (-1);
		}
	    } else {
		write_pos += num_write;
	    }
	}
    }

    /*
     * Read data from the network into the BIO pair.
     */
    while ((want_read = (int) BIO_ctrl_get_read_request(network_bio)) > 0) {
	if (want_read > (int) sizeof(buffer))
	    want_read = (int) sizeof(buffer);
	if (timeout > 0 && acl_read_wait(fd, timeout) < 0)
	    return (-1);
	num_read = acl_socket_read(fd, buffer, want_read, 0, 0, 0);
	if (num_read == 0)
	    /* FIX 200412 Cannot return a zero read count. */
	    return (-1);
	if (num_read < 0) {
	    if ((num_read < 0) && (timeout > 0) && (errno == ACL_EAGAIN || errno == ACL_EINTR)) {
		acl_msg_warn("%s: read() returns EAGAIN on a readable file descriptor!", myname);
		acl_msg_warn("%s: pausing to avoid going into a tight select/write loop!", myname);
		sleep(1);
	    } else {
		acl_msg_warn("%s: error reading %d bytes from the network: %s",
			myname, want_read, acl_last_serror());
		return (-1);
	    }
	} else {
	    to_bio = BIO_write(network_bio, buffer, num_read);
	    if (to_bio != num_read)
		acl_msg_panic("%s: BIO_write error: to_bio != num_read", myname);
	}
    }
    return (0);
}

/* tls_bio - perform SSL input/output operation with extreme prejudice */

int     tls_bio(ACL_SOCKET fd, int timeout, TLS_SESS_STATE *TLScontext,
	int (*hsfunc) (SSL *),
	int (*rfunc) (SSL *, void *, int),
	int (*wfunc) (SSL *, const void *, int),
	void *buf, int num)
{
    const char *myname = "tls_bio";
    int     status = 0;
    int     err;
    int     retval = 0;
    int     biop_retval;
    int     done;

    /*
     * If necessary, retry the SSL handshake or read/write operation after
     * handling any pending network I/O.
     */
    for (done = 0; done == 0; /* void */ ) {
	if (hsfunc) {
#if 1
	    status = hsfunc(TLScontext->con);
#else
	    status = SSL_do_handshake(TLScontext->con);
#endif
	} else if (rfunc)
	    status = rfunc(TLScontext->con, buf, num);
	else if (wfunc)
	    status = wfunc(TLScontext->con, buf, num);
	else
	    acl_msg_panic("%s: nothing to do here", myname);
	err = SSL_get_error(TLScontext->con, status);

#if (OPENSSL_VERSION_NUMBER <= 0x0090581fL)

	/*
	 * There is a bug up to and including OpenSSL-0.9.5a: if an error
	 * occurs while checking the peers certificate due to some
	 * certificate error (e.g. as happend with a RSA-padding error), the
	 * error is put onto the error stack. If verification is not
	 * enforced, this error should be ignored, but the error-queue is not
	 * cleared, so we can find this error here. The bug has been fixed on
	 * May 28, 2000.
	 * 
	 * This bug so far has only manifested as 4800:error:0407006A:rsa
	 * routines:RSA_padding_check_PKCS1_type_1:block type is not
	 * 01:rsa_pk1.c:100: 4800:error:04067072:rsa
	 * routines:RSA_EAY_PUBLIC_DECRYPT:padding check
	 * failed:rsa_eay.c:396: 4800:error:0D079006:asn1 encoding
	 * routines:ASN1_verify:bad get asn1 object call:a_verify.c:109: so
	 * that we specifically test for this error. We print the errors to
	 * the logfile and automatically clear the error queue. Then we retry
	 * to get another error code. We cannot do better, since we can only
	 * retrieve the last entry of the error-queue without actually
	 * cleaning it on the way.
	 * 
	 * This workaround is secure, as verify_result is set to "failed"
	 * anyway.
	 */
	if (err == SSL_ERROR_SSL) {
	    if (ERR_peek_error() == 0x0407006AL) {
		tls_print_errors();
		acl_msg_info("OpenSSL <= 0.9.5a workaround called: certificate errors ignored");
		err = SSL_get_error(TLScontext->con, status);
	    }
	}
#endif

	/*
	 * Find out if we must retry the operation and/or if there is pending
	 * network I/O.
	 * 
	 * XXX If we're the first to invoke SSL_shutdown(), then the operation
	 * isn't really complete when the call returns. We could hide that
	 * anomaly here and repeat the call.
	 */
	switch (err) {
	case SSL_ERROR_NONE:			/* success */
	    retval = status;
	    done = 1;
	    /* FALLTHROUGH */
	case SSL_ERROR_WANT_WRITE:		/* flush/update buffers */
	case SSL_ERROR_WANT_READ:
	    biop_retval = network_biopair_interop(fd, timeout, TLScontext->network_bio);
	    if (biop_retval < 0)
		return (-1);		/* network read/write error */
	    break;

	    /*
	     * With tls_timed_read() and tls_timed_write() the caller is the
	     * VSTREAM library module which is unaware of TLS, so we log the
	     * TLS error stack here. In a better world, each VSTREAM I/O
	     * object would provide an error reporting method in addition to
	     * the timed_read and timed_write methods, so that we would not
	     * need to have ad-hoc code like this.
	     */
	case SSL_ERROR_SSL:
	    if (rfunc || wfunc)
		tls_print_errors();
	    /* FALLTHROUGH */
	default:
	    retval = status;
	    done = 1;
	    break;
	}
    }
    return (retval);
}

#endif
