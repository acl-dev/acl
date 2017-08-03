#include "acl_stdafx.hpp"

#ifdef HAS_POLARSSL_DLL
# ifndef HAS_POLARSSL
#  define HAS_POLARSSL
# endif
#endif

#ifdef HAS_POLARSSL
# include "polarssl/ssl.h"
# include "polarssl/entropy.h"
# include "polarssl/certs.h"
# ifdef POLARSSL_1_3_X
#  include "polarssl/x509_crt.h"
# else
#  include "polarssl/x509.h"
# endif
# include "polarssl/ssl_cache.h"
#endif

#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"
#endif

#if defined(HAS_POLARSSL)

# ifdef POLARSSL_1_3_X
#  define X509_CRT		x509_crt
#  define PKEY			pk_context
# else
#  define X509_CRT		x509_cert
#  define PKEY			rsa_context
# endif

# ifdef HAS_POLARSSL_DLL
#  if defined(POLARSSL_1_3_X)
typedef void (*X509_INIT_FN)(X509_CRT*);
typedef void (*PK_INIT_FN)(PKEY*);
typedef void (*RSA_INIT_FN)(PKEY*, RSR_PKCS_V15, 0);
#  else
typedef void (*RSA_INIT_FN)(PKEY*, int, int);
#   define x509_init_fn		(void)
#   define entropy_free_fn	(void)
#  endif
typedef void (*ENTROPY_INIT_FN)(entropy_context*);
typedef void (*X509_FREE_FN)(X509_CRT*);
typedef int  (*X509_PARSE_CRTPATH_FN)(X509_CRT*, const char*);
typedef int  (*X509_PARSE_CRTFILE_FN)(X509_CRT*, const char*);
typedef int  (*X509_PARSE_KEYFILE_FN)(PKEY*, const char*, const char*);
typedef void (*PKEY_FREE_FN)(PKEY*);
typedef void (*SSL_SET_AUTHMODE_FN)(ssl_context*, int);
typedef void (*SSL_LIST_CIPHERSUITES_FN)(void);
typedef void (*SSL_SET_CIPHERSUITES_FN)(ssl_context*, const int*);
typedef void (*SSL_SET_SESSION_CACHE_FN)(ssl_context*,
        int (*f_get_cache)(void*, ssl_session*), void*,
        int (*f_set_cache)(void*, const ssl_session*), void*);
typedef void (*SSL_SET_CA_CHAIN_FN)(ssl_context*, X509_CRT*,
                       x509_crl*, const char*);
typedef void (*SSL_SET_OWN_CERT_FN)(ssl_context*, X509_CRT*, PKEY*);
typedef void (*SSL_CACHE_INIT_FN)(ssl_cache_context*);
typedef void (*SSL_CACHE_FREE_FN)(ssl_cache_context*);

static RSA_INIT_FN		rsa_init_fn;
static ENTROPY_INIT_FN		entropy_init_fn;
static X509_FREE_FN		x509_free_fn;
static X509_PARSE_CRTPATH_FN	x509_parse_crtpath_fn;
static X509_PARSE_CRTFILE_FN	x509_parse_crtfile_fn;
static X509_PARSE_KEYFILE_FN	x509_parse_keyfile_fn;
static PKEY_FREE_FN		pkey_free_fn;
static SSL_SET_AUTHMODE_FN	ssl_set_authmode_fn;
static SSL_LIST_CIPHERSUITES_FN	ssl_list_ciphersuites_fn;
static SSL_SET_CIPHERSUITES_FN	ssl_set_ciphersuites_fn;
static SSL_SET_SESSION_CACHE_FN	ssl_set_session_cache_fn;
static SSL_SET_CA_CHAIN_FN	ssl_set_ca_chain_fn;
static SSL_SET_OWN_CERT_FN	ssl_set_own_cert_fn;
static SSL_CACHE_INIT_FN	ssl_cache_init_fn;
static SSL_CACHE_FREE_FN	ssl_cache_free_fn;
# elif defined(POLARSSL_1_3_X)
#  define entropy_init_fn	::entropy_init
#  define entropy_free_fn	::entropy_free
#  define pk_init_fn		::pk_init_fn
#  define x509_init_fn		::x509_crt_init
#  define x509_free_fn		::x509_crt_free
#  define x509_parse_crtpath_fn	::x509_crt_parse_path
#  define x509_parse_crtfile_fn	::x509_crt_parse_file
#  define x509_parse_keyfile_fn	::pk_parse_keyfile
#  define pkey_free_fn		::pk_free
# else
#  define entropy_init_fn	::entropy_init
#  define entropy_free_fn	(void)
#  define rsa_init_fn		::rsa_init
#  define x509_init_fn		(void)
#  define x509_free_fn		::x509_free
#  define x509_parse_crtpath_fn	::x509parse_crtpath
#  define x509_parse_crtfile_fn	::x509parse_crtfile
#  define x509_parse_keyfile_fn	::x509parse_keyfile
#  define pkey_free_fn		::rsa_free
# endif

# define ssl_cache_init_fn	::ssl_cache_init
# define ssl_cache_free_fn	::ssl_cache_free
# define ssl_list_ciphersuites_fn ::ssl_list_ciphersuites
# define ssl_set_ciphersuites_fn  ::ssl_set_ciphersuites
# define ssl_set_session_cache_fn ::ssl_set_session_cache
# define ssl_set_ca_chain_fn	::ssl_set_ca_chain
# define ssl_set_own_cert_fn	::ssl_set_own_cert
# define ssl_set_authmode_fn	::ssl_set_authmode

#endif

namespace acl
{

polarssl_conf::polarssl_conf()
{
#ifdef HAS_POLARSSL
	entropy_ = acl_mycalloc(1, sizeof(entropy_context));
	entropy_init_fn((entropy_context*) entropy_);

	cacert_ = NULL;
	cert_chain_ = NULL;
	
	cache_ = NULL;
	pkey_ = NULL;
	verify_mode_ = POLARSSL_VERIFY_NONE;
#else
	(void) entropy_;
	(void) cacert_;
	(void) cert_chain_;
	(void) cache_;
	(void) pkey_;
	(void) verify_mode_;
#endif
}

polarssl_conf::~polarssl_conf()
{
#ifdef HAS_POLARSSL
	free_ca();

	if (cert_chain_)
	{
		x509_free_fn((X509_CRT*) cert_chain_);
		acl_myfree(cert_chain_);
	}

	if (pkey_)
	{
		pkey_free_fn((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	entropy_free_fn((entropy_context*) entropy_);
	acl_myfree(entropy_);

	if (cache_)
	{
		ssl_cache_free_fn((ssl_cache_context*) cache_);
		acl_myfree(cache_);
	}
#endif
}

void polarssl_conf::free_ca()
{
#ifdef HAS_POLARSSL
	if (cacert_) {
		x509_free_fn((X509_CRT*) cacert_);
		acl_myfree(cacert_);
		cacert_ = NULL;
	}
#endif
}

bool polarssl_conf::load_ca(const char* ca_file, const char* ca_path)
{
#ifdef HAS_POLARSSL
	free_ca();

	int  ret;

	cacert_ = acl_mycalloc(1, sizeof(X509_CRT));
	x509_init_fn((X509_CRT*) cacert_);

	if (ca_path && *ca_path)
	{
		ret = x509_parse_crtpath_fn((X509_CRT*) cacert_, ca_path);
		if (ret != 0)
		{
			logger_error("x509_crt_parse_path(%s) error: -0x%04x",
				ca_path, ret);
			free_ca();
			return false;
		}
	}

	if (ca_file == NULL || *ca_file == 0)
	{
		logger_error("ca_file null");
		free_ca();
		return false;
	}

	ret = x509_parse_crtfile_fn((X509_CRT*) cacert_, ca_file);
	if (ret != 0)
	{
		logger_error("x509_crt_parse_path(%s) error: -0x%04x",
			ca_path, ret);
		free_ca();
		return false;
	}
	else
		return true;
#else
	(void) ca_file;
	(void) ca_path;

	logger_error("HAS_POLARSSL not defined!");
	return false;
#endif
}

bool polarssl_conf::add_cert(const char* crt_file)
{
	if (crt_file == NULL || *crt_file == 0)
	{
		logger_error("crt_file null");
		return false;
	}

#ifdef HAS_POLARSSL
	if (cert_chain_ == NULL)
	{
		cert_chain_ = acl_mycalloc(1, sizeof(X509_CRT));
		x509_init_fn((X509_CRT*) cert_chain_);
	}

	int ret = x509_parse_crtfile_fn((X509_CRT*) cert_chain_, crt_file);
	if (ret != 0)
	{
		logger_error("x509_crt_parse_file(%s) error: -0x%04x",
			crt_file, ret);

		x509_free_fn((X509_CRT*) cert_chain_);
		acl_myfree(cert_chain_);
		cert_chain_ = NULL;
		return false;
	}

	return true;
#else
	(void) crt_file;
	logger_error("HAS_POLARSSL not defined!");
	return false;
#endif
}

bool polarssl_conf::set_key(const char* key_file,
	const char* key_pass /* = NULL */)
{
#ifdef HAS_POLARSSL
	if (pkey_ != NULL)
	{
		pkey_free_fn((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	pkey_ = acl_mycalloc(1, sizeof(PKEY));

# ifdef POLARSSL_1_3_X
	pk_init_fn((PKEY*) pkey_);
# else
	rsa_init_fn((PKEY*) pkey_, RSA_PKCS_V15, 0);
# endif

	int ret = x509_parse_keyfile_fn((PKEY*) pkey_, key_file,
			key_pass ? key_pass : "");
	if (ret != 0)
	{
		logger_error("pk_parse_keyfile(%s) error: -0x%04x",
			key_file, ret);

		pkey_free_fn((PKEY*) pkey_);
		acl_myfree(pkey_);
		pkey_ = NULL;
		return false;
	}

	return true;
#else
	(void) key_file;
	(void) key_pass;

	logger_error("HAS_POLARSSL not defined!");
	return false;
#endif
}

void polarssl_conf::set_authmode(polarssl_verify_t verify_mode)
{
	verify_mode_ = verify_mode;
}

void polarssl_conf::enable_cache(bool on)
{
#ifdef HAS_POLARSSL
	if (on)
	{
		if (cache_ != NULL)
			return;
		cache_ = acl_mycalloc(1, sizeof(ssl_cache_context));
		ssl_cache_init_fn((ssl_cache_context*) cache_);
	}
	else if (cache_ != NULL)
	{
		ssl_cache_free_fn((ssl_cache_context*) cache_);
		acl_myfree(cache_);
		cache_ = NULL;
	}
#else
	(void) on;
	logger_error("HAS_POLARSSL not defined!");
#endif
}

bool polarssl_conf::setup_certs(void* ssl_in, bool server_side)
{
#ifdef HAS_POLARSSL
	ssl_context* ssl = (ssl_context*) ssl_in;

	switch (verify_mode_)
	{
	case POLARSSL_VERIFY_NONE:
		ssl_set_authmode_fn((ssl_context*) ssl, SSL_VERIFY_NONE);
		break;
	case POLARSSL_VERIFY_OPT:
		ssl_set_authmode_fn((ssl_context*) ssl, SSL_VERIFY_OPTIONAL);
		break;
	case POLARSSL_VERIFY_REQ:
		ssl_set_authmode_fn((ssl_context*) ssl, SSL_VERIFY_REQUIRED);
		break;
	default:
		ssl_set_authmode_fn((ssl_context*) ssl, SSL_VERIFY_NONE);
		break;
	}

	// Setup cipher_suites
	const int* cipher_suites = ssl_list_ciphersuites_fn();
	if (cipher_suites == NULL)
	{
		logger_error("ssl_list_ciphersuites null");
		return false;
	}
	ssl_set_ciphersuites_fn((ssl_context*) ssl, cipher_suites);

//	::ssl_set_min_version((ssl_context*) ssl, SSL_MAJOR_VERSION_3,
//		SSL_MINOR_VERSION_0);
//	::ssl_set_renegotiation((ssl_context*) ssl, SSL_RENEGOTIATION_DISABLED);
//	::ssl_set_dh_param((ssl_context*) &ssl, POLARSSL_DHM_RFC5114_MODP_2048_P,
//		POLARSSL_DHM_RFC5114_MODP_2048_G );

	// Setup cache only for server-side
	if (server_side && cache_ != NULL)
		ssl_set_session_cache_fn(ssl, ssl_cache_get,
			(ssl_cache_context*) cache_, ssl_cache_set,
			(ssl_cache_context*) cache_);

	// Setup ca cert
	if (cacert_)
		ssl_set_ca_chain_fn(ssl, (X509_CRT*) cacert_, NULL, NULL);

	// Setup own's cert chain and private key

	if (cert_chain_ && pkey_)
	{
# ifdef POLARSSL_1_3_X
		int ret = ssl_set_own_cert_fn(ssl, (X509_CRT*) cert_chain_,
				(PKEY*) pkey_);
		if (ret != 0)
		{
			logger_error("ssl_set_own_cert error: -0x%04x", ret);
			return false;
		}
# else
		ssl_set_own_cert_fn(ssl, (X509_CRT*) cert_chain_, (PKEY*) pkey_);
# endif
	}

	return true;
#else
	(void) ssl_in;
	(void) server_side;
	logger_error("HAS_POLARSSL not defined!");
	return false;
#endif
}

} // namespace acl
