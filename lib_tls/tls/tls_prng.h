#ifndef _TLS_PRNG_SRC_H_INCLUDED_
#define _TLS_PRNG_SRC_H_INCLUDED_

/*++
 * NAME
 *	tls_prng_src 3h
 * SUMMARY
 *	OpenSSL PRNG maintenance routines
 * SYNOPSIS
 *	#include <tls_prng_src.h>
 * DESCRIPTION
 * .nf

  *
  * External interface.
  */
typedef struct TLS_PRNG_SRC {
	union {
		ACL_SOCKET     sock;				/* file handle */
		ACL_FILE_HANDLE file;
	} fd;
    char   *name;			/* resource name */
    int     timeout;			/* time limit of applicable */
} TLS_PRNG_SRC;

extern TLS_PRNG_SRC *tls_prng_egd_open(const char *, int);
extern ssize_t tls_prng_egd_read(TLS_PRNG_SRC *, size_t);
extern int tls_prng_egd_close(TLS_PRNG_SRC *);

extern TLS_PRNG_SRC *tls_prng_dev_open(const char *, int);
extern ssize_t tls_prng_dev_read(TLS_PRNG_SRC *, size_t);
extern int tls_prng_dev_close(TLS_PRNG_SRC *);

extern TLS_PRNG_SRC *tls_prng_file_open(const char *, int);
extern ssize_t tls_prng_file_read(TLS_PRNG_SRC *, size_t);
extern int tls_prng_file_close(TLS_PRNG_SRC *);

extern TLS_PRNG_SRC *tls_prng_exch_open(const char *);
extern void tls_prng_exch_update(TLS_PRNG_SRC *);
extern void tls_prng_exch_close(TLS_PRNG_SRC *);

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

#endif
