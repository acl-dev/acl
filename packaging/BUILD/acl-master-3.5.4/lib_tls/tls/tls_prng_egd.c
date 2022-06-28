/*++
 * NAME
 *	tls_prng_egd 3
 * SUMMARY
 *	seed OpenSSL PRNG from EGD server
 * SYNOPSIS
 *	#include <tls_prng_src.h>
 *
 *	TLS_PRNG_SRC *tls_prng_egd_open(name, timeout)
 *	const char *name;
 *	int	timeout;
 *
 *	ssize_t tls_prng_egd_read(egd, length)
 *	TLS_PRNG_SRC *egd;
 *	size_t length;
 *
 *	int	tls_prng_egd_close(egd)
 *	TLS_PRNG_SRC *egd;
 * DESCRIPTION
 *	tls_prng_egd_open() connect to the specified UNIX-domain service
 *	and returns a handle that should be used with all subsequent
 *	access.
 *
 *	tls_prng_egd_read() reads the requested number of bytes from
 *	the EGD server and updates the OpenSSL PRNG.
 *
 *	tls_prng_egd_close() disconnects from the specified EGD server
 *	and releases memory that was allocated for the handle.
 *
 *	Arguments:
 * .IP name
 *	The UNIX-domain pathname of the EGD service.
 * .IP length
 *	The number of bytes to read from the EGD server.
 *	Request lengths will be truncated at 255 bytes.
 * .IP timeout
 *	Time limit on individual I/O operations.
 * DIAGNOSTICS
 *	tls_prng_egd_open() returns a null pointer on error.
 *
 *	tls_prng_egd_read() returns -1 on error, the number
 *	of bytes received on success.
 *
 *	tls_prng_egd_close() returns -1 on error, 0 on success.
 *
 *	In all cases the errno variable indicates the type of error.
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

#include "StdAfx.h"
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <limits.h>

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */

/* TLS library. */

#include "tls_prng.h"

/* tls_prng_egd_open - connect to EGD server */

TLS_PRNG_SRC *tls_prng_egd_open(const char *name, int timeout)
{
    const char *myname = "tls_prng_egd_open";
    TLS_PRNG_SRC *egd;
    ACL_SOCKET     fd;

    if (acl_msg_verbose)
	acl_msg_info("%s: connect to EGD server %s", myname, name);

#ifdef ACL_UNIX
    fd = acl_unix_connect(name, ACL_BLOCKING, timeout);
#elif defined(WIN32)
	fd = acl_inet_connect(name, ACL_BLOCKING, timeout);
#endif
	
	if (fd < 0) {
	if (acl_msg_verbose)
	    acl_msg_info("%s: cannot connect to EGD server %s: %s",
		myname, name, acl_last_serror());
	return (0);
    } else {
	egd = (TLS_PRNG_SRC *) acl_mymalloc(sizeof(*egd));
	egd->fd.sock = fd;
	egd->name = acl_mystrdup(name);
	egd->timeout = timeout;
	if (acl_msg_verbose)
	    acl_msg_info("%s: connected to EGD server %s", myname, name);
	return (egd);
    }
}

/* tls_prng_egd_read - update internal PRNG from EGD server */

ssize_t tls_prng_egd_read(TLS_PRNG_SRC *egd, size_t len)
{
    const char *myname = "tls_prng_egd_read";
    unsigned char buffer[UCHAR_MAX];
    ssize_t count;

    if (len <= 0)
	acl_msg_panic("%s: bad length %ld", myname, (long) len);

    buffer[0] = 1;
    buffer[1] = (unsigned char) (len > UCHAR_MAX ? UCHAR_MAX : len);

    if (acl_timed_write(egd->fd.sock, buffer, 2, egd->timeout, (void *) 0) != 2) {
	acl_msg_info("%s: cannot write to EGD server %s: %s",
		myname, egd->name, acl_last_serror());
	return (-1);
    }
    if (acl_timed_read(egd->fd.sock, buffer, 1, egd->timeout, (void *) 0) != 1) {
	acl_msg_info("%s: cannot read from EGD server %s: %s",
		myname, egd->name, acl_last_serror());
	return (-1);
    }
    count = buffer[0];
    if (count > (ssize_t) sizeof(buffer))
	count = sizeof(buffer);
    if (count == 0) {
	acl_msg_info("EGD server %s reports zero bytes available", egd->name);
	return (-1);
    }
    if (acl_timed_read(egd->fd.sock, buffer, count, egd->timeout, (void *) 0) != count) {
	acl_msg_info("%s: cannot read %ld bytes from EGD server %s: %s",
		 myname, (long) count, egd->name, acl_last_serror());
	return (-1);
    }
    if (acl_msg_verbose)
	acl_msg_info("%s: got %ld bytes from EGD server %s", myname,
		 (long) count, egd->name);
    RAND_seed(buffer, count);
    return (count);
}

/* tls_prng_egd_close - disconnect from EGD server */

int     tls_prng_egd_close(TLS_PRNG_SRC *egd)
{
    const char *myname = "tls_prng_egd_close";
    int     err;

    if (acl_msg_verbose)
	acl_msg_info("%s: close EGD server %s", myname, egd->name);
    err = acl_file_close(egd->fd.file);
    acl_myfree(egd->name);
    acl_myfree(egd);
    return (err);
}

#endif
