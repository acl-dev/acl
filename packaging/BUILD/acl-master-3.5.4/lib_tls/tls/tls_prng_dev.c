/*++
 * NAME
 *	tls_prng_dev 3
 * SUMMARY
 *	seed OpenSSL PRNG from entropy device
 * SYNOPSIS
 *	#include <tls_prng_src.h>
 *
 *	TLS_PRNG_SRC *tls_prng_dev_open(name, timeout)
 *	const char *name;
 *	int	timeout;
 *
 *	ssize_t tls_prng_dev_read(dev, length)
 *	TLS_PRNG_SRC *dev;
 *	size_t length;
 *
 *	int	tls_prng_dev_close(dev)
 *	TLS_PRNG_SRC *dev;
 * DESCRIPTION
 *	tls_prng_dev_open() opens the specified entropy device
 *	and returns a handle that should be used with all subsequent
 *	access.
 *
 *	tls_prng_dev_read() reads the requested number of bytes from
 *	the entropy device and updates the OpenSSL PRNG.
 *
 *	tls_prng_dev_close() closes the specified entropy device
 *	and releases memory that was allocated for the handle.
 *
 *	Arguments:
 * .IP name
 *	The pathname of the entropy device.
 * .IP length
 *	The number of bytes to read from the entropy device.
 *	Request lengths will be truncated at 255 bytes.
 * .IP timeout
 *	Time limit on individual I/O operations.
 * DIAGNOSTICS
 *	tls_prng_dev_open() returns a null pointer on error.
 *
 *	tls_prng_dev_read() returns -1 on error, the number
 *	of bytes received on success.
 *
 *	tls_prng_dev_close() returns -1 on error, 0 on success.
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
#include <fcntl.h>
#ifdef	ACL_UNIX
#include <unistd.h>
#endif
#include <limits.h>
#include <errno.h>

#ifndef UCHAR_MAX
#define UCHAR_MAX 0xff
#endif

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */

/* TLS library. */

#include "tls_prng.h"

/* tls_prng_dev_open - open entropy device */

TLS_PRNG_SRC *tls_prng_dev_open(const char *name, int timeout)
{
    const char *myname = "tls_prng_dev_open";
    TLS_PRNG_SRC *dev;
    ACL_FILE_HANDLE     fd;

    if ((fd = acl_file_open(name, O_RDONLY, 0)) < 0) {
	if (acl_msg_verbose)
	    acl_msg_info("%s: cannot open entropy device %s: %s",
		myname, name, acl_last_serror());
	return (0);
    } else {
	dev = (TLS_PRNG_SRC *) acl_mymalloc(sizeof(*dev));
	dev->fd.file = fd;
	dev->name = acl_mystrdup(name);
	dev->timeout = timeout;
	if (acl_msg_verbose)
	    acl_msg_info("%s: opened entropy device %s", myname, name);
	return (dev);
    }
}

/* tls_prng_dev_read - update internal PRNG from device */

ssize_t tls_prng_dev_read(TLS_PRNG_SRC *dev, size_t len)
{
    const char *myname = "tls_prng_dev_read";
    unsigned char buffer[UCHAR_MAX];
    ssize_t count;
    size_t  rand_bytes;

    if (len <= 0)
	acl_msg_panic("%s: bad read length: %ld", myname, (long) len);

    if (len > sizeof(buffer))
	rand_bytes = sizeof(buffer);
    else
	rand_bytes = len;
    errno = 0;
#ifdef ACL_UNIX
    count = acl_timed_read(dev->fd.file, buffer, (int) rand_bytes, dev->timeout,
		NULL);
#elif defined(WIN32)
	count = acl_file_read(dev->fd.file, buffer, (int) rand_bytes,
		dev->timeout, NULL, NULL);
#endif
    if (count > 0) {
	if (acl_msg_verbose)
	    acl_msg_info("%s: read %ld bytes from entropy device %s",
		     myname, (long) count, dev->name);
	RAND_seed(buffer, count);
    } else {
	if (acl_msg_verbose)
	    acl_msg_info("%s: cannot read %ld bytes from entropy device %s: %s",
		     myname, (long) rand_bytes, dev->name, acl_last_serror());
    }
    return (count);
}

/* tls_prng_dev_close - disconnect from EGD server */

int     tls_prng_dev_close(TLS_PRNG_SRC *dev)
{
    const char *myname = "tls_prng_dev_close";
    int     err;

    if (acl_msg_verbose)
	acl_msg_info("%s: close entropy device %s", myname, dev->name);
    err = acl_file_close(dev->fd.file);
    acl_myfree(dev->name);
    acl_myfree(dev);
    return (err);
}

#endif
