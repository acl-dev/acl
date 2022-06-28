/*++
 * NAME
 *	tls_prng_exch 3
 * SUMMARY
 *	maintain PRNG exchange file
 * SYNOPSIS
 *	#include <tls_prng_src.h>
 *
 *	TLS_PRNG_SRC *tls_prng_exch_open(name, timeout)
 *	const char *name;
 *	int	timeout;
 *
 *	void	tls_prng_exch_update(fh, length)
 *	TLS_PRNG_SRC *fh;
 *	size_t length;
 *
 *	void	tls_prng_exch_close(fh)
 *	TLS_PRNG_SRC *fh;
 * DESCRIPTION
 *	tls_prng_exch_open() opens the specified PRNG exchange file
 *	and returns a handle that should be used with all subsequent
 *	access.
 *
 *	tls_prng_exch_update() reads the requested number of bytes
 *	from the PRNG exchange file, updates the OpenSSL PRNG, and
 *	writes the requested number of bytes to the exchange file.
 *	The file is locked for exclusive access.
 *
 *	tls_prng_exch_close() closes the specified PRNG exchange
 *	file and releases memory that was allocated for the handle.
 *
 *	Arguments:
 * .IP name
 *	The name of the PRNG exchange file.
 * .IP length
 *	The number of bytes to read from/write to the entropy file.
 * .IP timeout
 *	Time limit on individual I/O operations.
 * DIAGNOSTICS
 *	All errors are fatal.
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
#ifdef ACL_UNIX
#include <unistd.h>
#endif
#include <limits.h>

/* OpenSSL library. */

#ifdef USE_TLS
#include <openssl/rand.h>		/* For the PRNG */

/* TLS library. */

#include "tls_prng.h"

/* Application specific. */

#define TLS_PRNG_EXCH_SIZE	1024	/* XXX Why not configurable? */

/* tls_prng_exch_open - open PRNG exchange file */

TLS_PRNG_SRC *tls_prng_exch_open(const char *name)
{
    const char *myname = "tls_prng_exch_open";
    TLS_PRNG_SRC *eh;
    ACL_FILE_HANDLE     fd;

    if ((fd = acl_file_open(name, O_RDWR | O_CREAT, 0600)) < 0)
	acl_msg_fatal("%s: cannot open PRNG exchange file %s: %s",
		myname, name, acl_last_serror());
    eh = (TLS_PRNG_SRC *) acl_mymalloc(sizeof(*eh));
    eh->fd.file = fd;
    eh->name = acl_mystrdup(name);
    eh->timeout = 0;
    if (acl_msg_verbose)
	acl_msg_info("%s: opened PRNG exchange file %s", myname, name);
    return (eh);
}

/* tls_prng_exch_update - update PRNG exchange file */

void    tls_prng_exch_update(TLS_PRNG_SRC *eh)
{
    const char *myname = "tls_prng_exch_update";
    unsigned char buffer[TLS_PRNG_EXCH_SIZE];
    ssize_t count;

    /*
     * Update the PRNG exchange file. Since other processes may have added
     * entropy, we use a read-stir-write cycle.
     */
    if (acl_myflock(eh->fd.file, ACL_INTERNAL_LOCK, ACL_FLOCK_OP_EXCLUSIVE) != 0)
	acl_msg_fatal("%s: cannot lock PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());
    if (acl_lseek(eh->fd.file, 0, SEEK_SET) < 0)
	acl_msg_fatal("%s: cannot seek PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());
    if ((count = acl_file_read(eh->fd.file, buffer, sizeof(buffer), 0, NULL, NULL)) < 0)
	acl_msg_fatal("%s: cannot read PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());

    if (count > 0)
	RAND_seed(buffer, count);
    RAND_bytes(buffer, sizeof(buffer));

    if (acl_lseek(eh->fd.file, 0, SEEK_SET) < 0)
	acl_msg_fatal("%s: cannot seek PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());
    if (acl_file_write(eh->fd.file, buffer, sizeof(buffer), 0, NULL, NULL) != sizeof(buffer))
	acl_msg_fatal("%s: cannot write PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());
    if (acl_myflock(eh->fd.file, ACL_INTERNAL_LOCK, ACL_FLOCK_OP_NONE) != 0)
	acl_msg_fatal("%s: cannot unlock PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());
}

/* tls_prng_exch_close - close PRNG exchange file */

void    tls_prng_exch_close(TLS_PRNG_SRC *eh)
{
    const char *myname = "tls_prng_exch_close";

    if (acl_file_close(eh->fd.file) < 0)
	acl_msg_fatal("%s: close PRNG exchange file %s: %s",
		myname, eh->name, acl_last_serror());
    if (acl_msg_verbose)
	acl_msg_info("%s: closed PRNG exchange file %s", myname, eh->name);
    acl_myfree(eh->name);
    acl_myfree(eh);
}

#endif
