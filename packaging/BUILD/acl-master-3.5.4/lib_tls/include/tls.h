#ifndef _TLS_H_INCLUDED_
#define _TLS_H_INCLUDED_

/*++
 * NAME
 *      tls 3h
 * SUMMARY
 *      libtls internal interfaces
 * SYNOPSIS
 *      #include <tls.h>
 * DESCRIPTION
 * .nf

  *
  * Utility library.
  */

#include "lib_acl.h"

#ifdef USE_TLS

#ifdef __cplusplus
extern "C" {
#endif

#ifdef TLS_DLL
# ifdef TLS_EXPORTS
#  define TLS_API __declspec(dllexport)
# else
#  define TLS_API __declspec(dllimport)
# endif
#else
#  define TLS_API
#endif

 /*
  * OpenSSL library.
  */
#include <openssl/lhash.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>

#if (OPENSSL_VERSION_NUMBER < 0x00905100L)
#error "need OpenSSL version 0.9.5 or later"
#endif

 /*
  * TLS enforcement levels. Non-sentinel values may also be used to indicate
  * the actual security level of a session.
  * 
  * XXX TLS_LEV_NOTFOUND no longer belongs in this list. The SMTP client will
  * have to use something else to report that policy table lookup failed.
  */
#define TLS_LEV_INVALID		-2	/* sentinel */
#define TLS_LEV_NOTFOUND	-1	/* XXX not in policy table */
#define TLS_LEV_NONE		0	/* plain-text only */
#define TLS_LEV_MAY		1	/* wildcard */
#define TLS_LEV_ENCRYPT		2	/* encrypted connection */
#define TLS_LEV_FPRINT		3	/* "peer" CA-less verification */
#define TLS_LEV_VERIFY		4	/* certificate verified */
#define TLS_LEV_SECURE		5	/* "secure" verification */

 /*
  * Peer status bits. TLS_CERT_FLAG_MATCHED implies TLS_CERT_FLAG_TRUSTED
  * only in the case of a hostname match.
  */
#define TLS_CERT_FLAG_PRESENT		(1<<0)
#define TLS_CERT_FLAG_ALTNAME		(1<<1)
#define TLS_CERT_FLAG_TRUSTED		(1<<2)
#define TLS_CERT_FLAG_MATCHED		(1<<3)
#define TLS_CERT_FLAG_LOGGED		(1<<4)	/* Logged trust chain error */

#define TLS_CERT_IS_PRESENT(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_PRESENT))
#define TLS_CERT_IS_ALTNAME(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_ALTNAME))
#define TLS_CERT_IS_TRUSTED(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_TRUSTED))
#define TLS_CERT_IS_MATCHED(c) ((c) && ((c)->peer_status&TLS_CERT_FLAG_MATCHED))


 /*
  * Names of valid tlsmgr(8) session caches.
  */
#define TLS_MGR_SCACHE_SERVER	"server"
#define TLS_MGR_SCACHE_CLIENT	"client"

 /*
  * TLS session context, also used by the ACL_VSTREAM call-back routines for SMTP
  * input/output, and by OpenSSL call-back routines for key verification.
  * Only some members are (read-only) accessible by the public.
  */
typedef struct TLS_SESS_STATE TLS_SESS_STATE;

struct TLS_SESS_STATE {
    /* Public, read-only. */
    char   *peer_CN;			/* Peer Common Name */
    char   *issuer_CN;			/* Issuer Common Name */
    char   *peer_fingerprint;		/* ASCII fingerprint */
    int     peer_status;		/* Certificate and match status */
    const char *protocol;
    const char *cipher_name;
    int     cipher_usebits;
    int     cipher_algbits;
    /* Private. */
    SSL    *con;
    BIO    *internal_bio;		/* postfix/TLS side of pair */
    BIO    *network_bio;		/* network side of pair */
    char   *cache_type;			/* tlsmgr(8) cache type if enabled */
    char   *serverid;			/* unique server identifier */
    char   *namaddr;			/* nam[addr] for logging */
    int     log_level;			/* TLS library logging level */
    int     session_reused;		/* this session was reused */
    int     am_server;			/* Are we an SSL server or client? */
};

 /*
  * Opaque client context handle.
  */
typedef struct TLS_APPL_STATE TLS_APPL_STATE;

 /*
  * tls_client.c
  */
typedef struct {
    int     log_level;
    int     verifydepth;
    const char *cache_type;
    const char *cert_file;
    const char *key_file;
    const char *dcert_file;
    const char *dkey_file;
    const char *eccert_file;
    const char *eckey_file;
    const char *CAfile;
    const char *CApath;
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_CLIENT_INIT_PROPS;

typedef struct {
    TLS_APPL_STATE *ctx;
    ACL_VSTREAM *stream;
    int     log_level;
    int     timeout;
    int     tls_level;			/* Security level */
    const char *nexthop;		/* destination domain */
    const char *host;			/* MX hostname */
    const char *namaddr;		/* nam[addr] for logging */
    const char *serverid;		/* Session cache key */
    const char *protocols;		/* Enabled protocols */
    const char *cipher_grade;		/* Minimum cipher grade */
    const char *cipher_exclusions;	/* Ciphers to exclude */
    const ACL_ARGV *matchargv;		/* Cert match patterns */
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_CLIENT_START_PROPS;

TLS_API void tls_client_init(void);
TLS_API int tls_client_setup(const TLS_CLIENT_INIT_PROPS *);
TLS_API TLS_APPL_STATE *tls_client_create(const TLS_CLIENT_INIT_PROPS *);
TLS_API TLS_SESS_STATE *tls_client_start(const TLS_CLIENT_START_PROPS *);

#define tls_client_stop(ctx, stream, timeout, failure, TLScontext) \
	tls_session_stop(ctx, (stream), (timeout), (failure), (TLScontext))

#define TLS_CLIENT_SETUP(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12) \
    tls_client_setup((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), (props)))

#define TLS_CLIENT_CREATE(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12) \
    tls_client_init((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), (props)))

#define TLS_CLIENT_START(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13, a14) \
    tls_client_start((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14), (props)))

 /*
  * tls_server.c
  */
typedef struct {
    int     log_level;
    int     verifydepth;
    const char *cache_type;
    long    scache_timeout;
    int     set_sessid;
    const char *cert_file;
    const char *key_file;
    const char *dcert_file;
    const char *dkey_file;
    const char *eccert_file;
    const char *eckey_file;
    const char *CAfile;
    const char *CApath;
    const char *protocols;
    const char *eecdh_grade;
    const char *dh1024_param_file;
    const char *dh512_param_file;
    int     ask_ccert;
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_SERVER_INIT_PROPS;

typedef struct {
    TLS_APPL_STATE *ctx;		/* TLS application context */
    ACL_VSTREAM *stream;		/* Client stream */
    int     log_level;			/* TLS log level */
    int     timeout;			/* TLS handshake timeout */
    int     requirecert;		/* Insist on client cert? */
    const char *serverid;		/* Server instance (salt cache key) */
    const char *namaddr;		/* Client nam[addr] for logging */
    const char *cipher_grade;
    const char *cipher_exclusions;
    const char *fpt_dgst;		/* Fingerprint digest algorithm */
} TLS_SERVER_START_PROPS;

TLS_API void tls_server_init(void);
TLS_API int tls_server_setup(const TLS_SERVER_INIT_PROPS *);
TLS_API TLS_APPL_STATE *tls_server_create(const TLS_SERVER_INIT_PROPS *);
TLS_API TLS_SESS_STATE *tls_server_start(const TLS_SERVER_START_PROPS *props);

#define tls_server_stop(ctx, stream, timeout, failure, TLScontext) \
	tls_session_stop(ctx, (stream), (timeout), (failure), (TLScontext))

#define TLS_SERVER_SETUP(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
    tls_server_setup((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14), ((props)->a15), \
    ((props)->a16), ((props)->a17), ((props)->a18), ((props)->a19), (props)))

#define TLS_SERVER_CREATE(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, \
    a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
    tls_server_init((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), ((props)->a11), \
    ((props)->a12), ((props)->a13), ((props)->a14), ((props)->a15), \
    ((props)->a16), ((props)->a17), ((props)->a18), ((props)->a19), (props)))

#define TLS_SERVER_START(props, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
    tls_server_start((((props)->a1), ((props)->a2), ((props)->a3), \
    ((props)->a4), ((props)->a5), ((props)->a6), ((props)->a7), \
    ((props)->a8), ((props)->a9), ((props)->a10), (props)))

 /*
  * tls_session.c
  */
TLS_API void tls_session_stop(TLS_APPL_STATE *, ACL_VSTREAM *, int, int, TLS_SESS_STATE *);

 /*
  * tls_misc.c One time finalization of application context.
  */
TLS_API void tls_free_app_context(TLS_APPL_STATE *);

 /*
  * tls_params.c
  */
TLS_API void tls_params_init(void);

 /*
  * tlsmgr_daemon.c
  */
TLS_API void tlsmgr_alone_start(int argc, char *argv[]);
TLS_API void tlsmgr_local_start(const char *addr);

/* LICENSE
 * .ad
 * .fi
 *      The Secure Mailer license must be distributed with this software.
 * AUTHOR(S)
 *      Wietse Venema
 *      IBM T.J. Watson Research
 *      P.O. Box 704
 *      Yorktown Heights, NY 10598, USA
 *
 *	Victor Duchovni
 *	Morgan Stanley
 *--*/

#ifdef __cplusplus
}
#endif
#endif					/* USE_TLS */
#endif					/* _TLS_H_INCLUDED_ */
