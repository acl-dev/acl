#ifndef	__TLS_PRIVATE_INCLUDE_H__
#define	__TLS_PRIVATE_INCLUDE_H__

#ifdef USE_TLS

#include "lib_acl.h"
#include "tls.h"
#include <openssl/ssl.h>
#include "../util/name_code.h"
#include "../util/name_mask.h"

#define TLS_BIO_BUFSIZE		8192
#define CCERT_BUFSIZ	256

extern const NAME_CODE tls_level_table[];

#define tls_level_lookup(s) name_code(tls_level_table, NAME_CODE_FLAG_NONE, (s))
#define str_tls_level(l) str_name_code(tls_level_table, (l))

 /*
  * Client and Server application contexts
  */
struct TLS_APPL_STATE {
    SSL_CTX *ssl_ctx;
    char   *cache_type;
    char   *cipher_exclusions;		/* Last cipher selection state */
    char   *cipher_list;		/* Last cipher selection state */
    int     cipher_grade;		/* Last cipher selection state */
    ACL_VSTRING *why;
};

 /*
  * Protocol selection.
  */
#define TLS_PROTOCOL_INVALID	(~0)	/* All protocol bits masked */
#define TLS_PROTOCOL_SSLv2	(1<<0)	/* SSLv2 */
#define TLS_PROTOCOL_SSLv3	(1<<1)	/* SSLv3 */
#define TLS_PROTOCOL_TLSv1	(1<<2)	/* TLSv1 */
#define TLS_KNOWN_PROTOCOLS	\
	( TLS_PROTOCOL_SSLv2 | TLS_PROTOCOL_SSLv3 | TLS_PROTOCOL_TLSv1 )

extern int tls_protocol_mask(const char *);

 /*
  * Cipher grade selection.
  */
#define TLS_CIPHER_NONE		0
#define TLS_CIPHER_NULL		1
#define TLS_CIPHER_EXPORT	2
#define TLS_CIPHER_LOW		3
#define TLS_CIPHER_MEDIUM	4
#define TLS_CIPHER_HIGH		5

extern const NAME_CODE tls_cipher_grade_table[];

#define tls_cipher_grade(str) \
    name_code(tls_cipher_grade_table, NAME_CODE_FLAG_NONE, (str))
#define str_tls_cipher_grade(gr) \
    str_name_code(tls_cipher_grade_table, (gr))

 /*
  * Cipher lists with exclusions.
  */
extern const char *tls_set_ciphers(TLS_APPL_STATE *, const char *,
				           const char *, const char *);

 /*
  * tls_session.c
  */
extern ACL_VSTRING *tls_session_passivate(SSL_SESSION *);
extern SSL_SESSION *tls_session_activate(const char *, int);

 /*
  * tls_stream.c.
  */
extern void tls_stream_start(ACL_VSTREAM *, TLS_SESS_STATE *);
extern void tls_stream_stop(ACL_VSTREAM *);

 /*
  * tls_bio_ops.c: a generic multi-personality driver that retries SSL
  * operations until they are satisfied or until a hard error happens.
  * Because of its ugly multi-personality user interface we invoke it via
  * not-so-ugly single-personality wrappers.
  */
extern int tls_bio(ACL_SOCKET, int, TLS_SESS_STATE *,
		           int (*) (SSL *),	/* handshake */
		           int (*) (SSL *, void *, int),	/* read */
		           int (*) (SSL *, const void *, int),	/* write */
		           void *, int);

#define tls_bio_connect(fd, timeout, context) \
        tls_bio((fd), (timeout), (context), SSL_connect, \
		NULL, NULL, NULL, 0)
#define tls_bio_accept(fd, timeout, context) \
        tls_bio((fd), (timeout), (context), SSL_accept, \
		NULL, NULL, NULL, 0)
#define tls_bio_shutdown(fd, timeout, context) \
	tls_bio((fd), (timeout), (context), SSL_shutdown, \
		NULL, NULL, NULL, 0)
#define tls_bio_read(fd, buf, len, timeout, context) \
	tls_bio((fd), (timeout), (context), NULL, \
		SSL_read, NULL, (buf), (len))
#define tls_bio_write(fd, buf, len, timeout, context) \
	tls_bio((fd), (timeout), (context), NULL, \
		NULL, SSL_write, (buf), (len))

 /*
  * tls_dh.c
  */
extern void tls_set_dh_from_file(const char *, int);
extern DH *tls_tmp_dh_cb(SSL *, int, int);
extern int tls_set_eecdh_curve(SSL_CTX *, const char *);

 /*
  * tls_rsa.c
  */
extern RSA *tls_tmp_rsa_cb(SSL *, int, int);

 /*
  * tls_verify.c
  */
extern char *tls_peer_CN(X509 *, const TLS_SESS_STATE *);
extern char *tls_issuer_CN(X509 *, const TLS_SESS_STATE *);
extern const char *tls_dns_name(const GENERAL_NAME *, const TLS_SESS_STATE *);
extern char *tls_fingerprint(X509 *, const char *);
extern int tls_verify_certificate_callback(int, X509_STORE_CTX *);

 /*
  * tls_certkey.c
  */
extern int tls_set_ca_certificate_info(SSL_CTX *, const char *, const char *);
extern int tls_set_my_certificate_key_info(SSL_CTX *,
				       /* RSA */ const char *, const char *,
				       /* DSA */ const char *, const char *,
				    /* ECDSA */ const char *, const char *);

 /*
  * tls_misc.c
  */
extern int TLScontext_index;

extern TLS_APPL_STATE *tls_alloc_app_context(SSL_CTX *);
extern TLS_SESS_STATE *tls_alloc_sess_context(int, const char *);
extern void tls_free_context(TLS_SESS_STATE *);
extern void tls_check_version(void);
extern long tls_bug_bits(void);
extern void tls_print_errors(void);
extern void tls_info_callback(const SSL *, int, int);
extern long tls_bio_dump_cb(BIO *, int, const char *, int, long, long);

 /*
  * tls_seed.c
  */
extern void tls_int_seed(void);
extern int tls_ext_seed(int);

 /*
  * tls_threads.c
  */
extern void tls_threads_init(void);

#endif

#endif
