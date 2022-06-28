#ifndef _TLS_SCACHE_H_INCLUDED_
#define _TLS_SCACHE_H_INCLUDED_

/*++
 * NAME
 *	tls_scache 3h
 * SUMMARY
 *	TLS session cache manager
 * SYNOPSIS
 *	#include <tls_scache.h>
 * DESCRIPTION
 * .nf

  *
  * Utility library.
  */
#include "lib_acl.h"
#include "dict.h"

 /*
  * External interface.
  */
typedef struct {
    int     flags;			/* see below */
    DICT   *db;				/* database handle */
    char   *cache_label;		/* "smtpd", "smtp" or "lmtp" */
    int     verbose;			/* enable verbose logging */
    int     timeout;			/* smtp(d)_tls_session_cache_timeout */
    char   *saved_cursor;		/* cursor cache ID */
} TLS_SCACHE;

#define TLS_SCACHE_FLAG_DEL_SAVED_CURSOR	(1<<0)

extern void tls_scache_init(void);
extern TLS_SCACHE *tls_scache_open(const char *, const char *, int, int);
extern void tls_scache_close(TLS_SCACHE *);
extern int tls_scache_lookup(TLS_SCACHE *, char *, ACL_VSTRING *);
extern int tls_scache_update(TLS_SCACHE *, char *, const char *, ssize_t);
extern int tls_scache_delete(TLS_SCACHE *, char *);
extern int tls_scache_sequence(TLS_SCACHE *, int, char **, ACL_VSTRING *);

#define TLS_SCACHE_DONT_NEED_CACHE_ID		((char **) 0)
#define TLS_SCACHE_DONT_NEED_SESSION		((ACL_VSTRING *) 0)

#define TLS_SCACHE_SEQUENCE_NOTHING \
	TLS_SCACHE_DONT_NEED_CACHE_ID, TLS_SCACHE_DONT_NEED_SESSION

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
