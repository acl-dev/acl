/*++
 * NAME
 *	tls_seed 3
 * SUMMARY
 *	TLS PRNG seeding routines
 * SYNOPSIS
 *	#include <tls.h>
 *	#include <tls_private.h>
 *
 *	int	tls_ext_seed(nbytes)
 *	int	nbytes;
 *
 *	void	tls_int_seed()
 * DESCRIPTION
 *	tls_ext_seed() requests the specified number of bytes
 *	from the tlsmgr(8) PRNG pool and updates the local PRNG.
 *	The result is zero in case of success, -1 otherwise.
 *
 *	tls_int_seed() mixes the process ID and time of day into
 *	the PRNG pool. This adds a few bits of entropy with each
 *	call, provided that the calls aren't made frequently.
 * LICENSE
 * .ad
 * .fi
 *	The Secure Mailer license must be distributed with this
 *	software.
 * AUTHOR(S)
 *	Wietse Venema
 *	IBM T.J. Watson Research
 *	P.O. Box 704
 *	Yorktown Heights, NY 10598, USA
 *--*/

#include "StdAfx.h"
#ifdef	ACL_UNIX
#include <sys/time.h>			/* gettimeofday() */
#include <unistd.h>			/* getpid() */
#elif defined(WIN32)
#include <process.h>
#endif

#ifdef USE_TLS

/* OpenSSL library. */

#include <openssl/rand.h>		/* RAND_seed() */

/* TLS library. */

#include "tls.h"
#include "tls_mgr.h"
#include "tls_private.h"

/* Application-specific. */

/* tls_int_seed - add entropy to the pool by adding the time and PID */

#ifdef WIN32
# define getpid _getpid
#endif

void    tls_int_seed(void)
{
    static __thread struct {
#ifdef WIN32
	int pid;
#else
	pid_t   pid;
#endif
	struct timeval tv;
    } randseed;

    if (randseed.pid == 0)
	randseed.pid = getpid();
    gettimeofday(&randseed.tv, NULL);
    RAND_seed(&randseed, sizeof(randseed));
}

/* tls_ext_seed - request entropy from tlsmgr(8) server */

int     tls_ext_seed(int nbytes)
{
    ACL_VSTRING *buf;
    int     status;

    buf = acl_vstring_alloc(nbytes);
    status = tls_mgr_seed(buf, nbytes);
    RAND_seed(acl_vstring_str(buf), (int) ACL_VSTRING_LEN(buf));
    acl_vstring_free(buf);
    return (status == TLS_MGR_STAT_OK ? 0 : -1);
}

#endif
