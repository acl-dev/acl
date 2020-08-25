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
# include "acl_cpp/stdlib/log.hpp"
# include "acl_cpp/stream/polarssl_io.hpp"
# include "acl_cpp/stream/polarssl_conf.hpp"
#endif

#if defined(HAS_POLARSSL)

# ifdef POLARSSL_1_3_X
#  define X509_CRT			x509_crt
#  define PKEY				pk_context
# else
#  define X509_CRT			x509_cert
#  define PKEY				rsa_context
# endif

# ifdef HAS_POLARSSL_DLL
#  if defined(POLARSSL_1_3_X)
typedef void (*x509_init_fn)(X509_CRT*);
typedef void (*pk_init_fn)(PKEY*);
typedef void (*rsa_init_fn)(PKEY*, RSR_PKCS_V15, 0);

#   define ENTROPY_FREE_NAME		"entropy_free"
#   define PK_INIT_NAME			"pk_init"
#   define X509_INIT_NAME		"x509_crt_init"
#   define X509_FREE_NAME		"x509_crt_free"
#   define X509_PARSE_CRTPATH_NAME	"x509_crt_parse_path"
#   define X509_PARSE_CRTFILE_NAME	"x509_crt_parse_file"
#   define X509_PARSE_KEYFILE_NAME	"pk_parse_keyfile"
#   define PKEY_FREE_NAME		"pk_free"
#  else
typedef void (*rsa_init_fn)(PKEY*, int, int);
#   define __x509_init			(void)
#   define __entropy_free		(void)

#   define RSA_INIT_NAME		"rsa_init"
#   define X509_FREE_NAME		"x509_free"
#   define X509_PARSE_CRTPATH_NAME	"x509parse_crtpath"
#   define X509_PARSE_CRTFILE_NAME	"x509parse_crtfile"
#   define X509_PARSE_KEYFILE_NAME	"x509parse_keyfile"
#   define PKEY_FREE_NAME		"rsa_free"
#  endif

#  define ENTROPY_INIT_NAME		"entropy_init"
#  define SSL_LIST_CIPHERSUITES_NAME	"ssl_list_ciphersuites"
#  define SSL_SET_CIPHERSUITES_NAME	"ssl_set_ciphersuites"
#  define SSL_SET_SESSION_CACHE_NAME	"ssl_set_session_cache"
#  define SSL_SET_CA_CHAIN_NAME		"ssl_set_ca_chain"
#  define SSL_SET_OWN_CERT_NAME		"ssl_set_own_cert"
#  define SSL_SET_AUTHMODE_NAME		"ssl_set_authmode"
#  define SSL_CACHE_INIT_NAME		"ssl_cache_init"
#  define SSL_CACHE_FREE_NAME		"ssl_cache_free"
#  define SSL_CACHE_SET_NAME		"ssl_cache_set"
#  define SSL_CACHE_GET_NAME		"ssl_cache_get"

typedef void (*entropy_init_fn)(entropy_context*);
typedef void (*x509_free_fn)(X509_CRT*);
typedef int  (*x509_parse_crtpath_fn)(X509_CRT*, const char*);
typedef int  (*x509_parse_crtfile_fn)(X509_CRT*, const char*);
typedef int  (*x509_parse_keyfile_fn)(PKEY*, const char*, const char*);
typedef void (*pkey_free_fn)(PKEY*);

typedef const int* (*ssl_list_ciphersuites_fn)(void);
typedef void (*ssl_set_ciphersuites_fn)(ssl_context*, const int*);
typedef void (*ssl_set_session_cache_fn)(ssl_context*,
        int (*f_get_cache)(void*, ssl_session*), void*,
        int (*f_set_cache)(void*, const ssl_session*), void*);
typedef void (*ssl_set_ca_chain_fn)(ssl_context*, X509_CRT*,
                       x509_crl*, const char*);
typedef void (*ssl_set_own_cert_fn)(ssl_context*, X509_CRT*, PKEY*);
typedef void (*ssl_set_authmode_fn)(ssl_context*, int);
typedef void (*ssl_cache_init_fn)(ssl_cache_context*);
typedef void (*ssl_cache_free_fn)(ssl_cache_context*);
typedef int (*ssl_cache_set_fn)(void*, const ssl_session*);;
typedef int (*ssl_cache_get_fn)(void*, ssl_session*);

static entropy_init_fn		__entropy_init;
static rsa_init_fn		__rsa_init;
static x509_free_fn		__x509_free;
static x509_parse_crtpath_fn	__x509_parse_crtpath;
static x509_parse_crtfile_fn	__x509_parse_crtfile;
static x509_parse_keyfile_fn	__x509_parse_keyfile;
static pkey_free_fn		__pkey_free;

static ssl_list_ciphersuites_fn	__ssl_list_ciphersuites;

static ssl_set_ciphersuites_fn	__ssl_set_ciphersuites;
static ssl_set_session_cache_fn	__ssl_set_session_cache;
static ssl_set_ca_chain_fn	__ssl_set_ca_chain;
static ssl_set_own_cert_fn	__ssl_set_own_cert;
static ssl_set_authmode_fn	__ssl_set_authmode;
static ssl_cache_init_fn	__ssl_cache_init;
static ssl_cache_free_fn	__ssl_cache_free;
static ssl_cache_set_fn		__ssl_cache_set;
static ssl_cache_get_fn		__ssl_cache_get;

static acl_pthread_once_t	__polarssl_once     = ACL_PTHREAD_ONCE_INIT;
static acl::string*		__polarssl_path_buf = NULL;
#ifdef	ACL_WINDOWS
static const char* __polarssl_path = "libpolarssl.dll";
#else
static const char* __polarssl_path = "./libpolarssl.so";
#endif

ACL_DLL_HANDLE			__polarssl_dll  = NULL;

#ifndef HAVE_NO_ATEXIT
static void polarssl_dll_unload(void)
{
	if (__polarssl_dll) {
		acl_dlclose(__polarssl_dll);
		__polarssl_dll = NULL;
		logger("%s unload ok", __polarssl_path);
	}

	delete __polarssl_path_buf;
	__polarssl_path_buf = NULL;
}
#endif

extern bool polarssl_dll_load_io(void); // defined in polarssl_io.cpp

static bool polarssl_dll_load_conf(void)
{
#define LOAD(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__polarssl_dll, (name));		\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s", name, acl_dlerror());	\
		return false;						\
	}								\
} while (0)

#if defined(POLARSSL_1_3_X)
	LOAD(X509_INIT_NAME, x509_init_fn, __x509_init);
	LOAD(PK_INIT_NAME, pk_init_fn, __pk_init);
#else
	LOAD(RSA_INIT_NAME, rsa_init_fn, __rsa_init);
#endif
	LOAD(ENTROPY_INIT_NAME, entropy_init_fn, __entropy_init);
	LOAD(X509_FREE_NAME, x509_free_fn, __x509_free);
	LOAD(X509_PARSE_CRTPATH_NAME, x509_parse_crtpath_fn, __x509_parse_crtpath);
	LOAD(X509_PARSE_CRTFILE_NAME, x509_parse_crtfile_fn, __x509_parse_crtfile);
	LOAD(X509_PARSE_KEYFILE_NAME, x509_parse_keyfile_fn, __x509_parse_keyfile);
	LOAD(PKEY_FREE_NAME, pkey_free_fn, __pkey_free);
	LOAD(SSL_LIST_CIPHERSUITES_NAME, ssl_list_ciphersuites_fn, __ssl_list_ciphersuites);
	LOAD(SSL_SET_CIPHERSUITES_NAME, ssl_set_ciphersuites_fn, __ssl_set_ciphersuites);
	LOAD(SSL_SET_SESSION_CACHE_NAME, ssl_set_session_cache_fn, __ssl_set_session_cache);
	LOAD(SSL_SET_CA_CHAIN_NAME, ssl_set_ca_chain_fn, __ssl_set_ca_chain);
	LOAD(SSL_SET_OWN_CERT_NAME, ssl_set_own_cert_fn, __ssl_set_own_cert);
	LOAD(SSL_SET_AUTHMODE_NAME, ssl_set_authmode_fn, __ssl_set_authmode);
	LOAD(SSL_CACHE_INIT_NAME, ssl_cache_init_fn, __ssl_cache_init);
	LOAD(SSL_CACHE_FREE_NAME, ssl_cache_free_fn, __ssl_cache_free);
	LOAD(SSL_CACHE_SET_NAME, ssl_cache_set_fn, __ssl_cache_set);
	LOAD(SSL_CACHE_GET_NAME, ssl_cache_get_fn, __ssl_cache_get);
	return true;
}

static void polarssl_dll_load(void)
{
	if (__polarssl_dll) {
		logger("polarssl(%s) has been loaded!", __polarssl_path);
		return;
	}

	if (__polarssl_path_buf != NULL && !__polarssl_path_buf->empty()) {
		__polarssl_path = __polarssl_path_buf->c_str();
	}

	__polarssl_dll = acl_dlopen(__polarssl_path);
	if (__polarssl_dll == NULL) {
		logger_error("load %s error %s", __polarssl_path, acl_dlerror());
		return;
	}

	if (!polarssl_dll_load_conf()) {
		logger_error("load %s error", __polarssl_path);
		acl_dlclose(__polarssl_dll);
		__polarssl_dll = NULL;
		return;
	}
	if (!polarssl_dll_load_io()) {
		logger_error("load %s error", __polarssl_path);
		acl_dlclose(__polarssl_dll);
		__polarssl_dll = NULL;
		return;
	}

	logger("%s loaded!", __polarssl_path);
#ifndef HAVE_NO_ATEXIT
	atexit(polarssl_dll_unload);
#endif
}

# else // !HAS_POLARSSL_DLL && HAS_POLARSSL

#  if defined(POLARSSL_1_3_X)
#   define __entropy_free	::entropy_free
#   define __pk_init		::pk_init
#   define __x509_init		::x509_crt_init
#   define __x509_free		::x509_crt_free
#   define __x509_parse_crtpath	::x509_crt_parse_path
#   define __x509_parse_crtfile	::x509_crt_parse_file
#   define __x509_parse_keyfile	::pk_parse_keyfile
#   define __pkey_free		::pk_free
#  else
#   define __entropy_free	(void)
#   define __rsa_init		::rsa_init
#   define __x509_init		(void)
#   define __x509_free		::x509_free
#   define __x509_parse_crtpath	::x509parse_crtpath
#   define __x509_parse_crtfile	::x509parse_crtfile
#   define __x509_parse_keyfile	::x509parse_keyfile
#   define __pkey_free		::rsa_free
#  endif

#  define __entropy_init	::entropy_init
#  define __ssl_list_ciphersuites ::ssl_list_ciphersuites
#  define __ssl_set_ciphersuites  ::ssl_set_ciphersuites
#  define __ssl_set_session_cache ::ssl_set_session_cache
#  define __ssl_set_ca_chain	::ssl_set_ca_chain
#  define __ssl_set_own_cert	::ssl_set_own_cert
#  define __ssl_set_authmode	::ssl_set_authmode
#  define __ssl_cache_init	::ssl_cache_init
#  define __ssl_cache_free	::ssl_cache_free
#  define __ssl_cache_set	::ssl_cache_set
#  define __ssl_cache_get	::ssl_cache_get

# endif // HAS_POLARSSL_DLL
#endif  // HAS_POLARSSL

namespace acl
{

void polarssl_conf::set_libpath(const char* path acl_unused)
{
#ifdef HAS_POLARSSL_DLL
	if (__polarssl_path_buf == NULL) {
		__polarssl_path_buf = NEW string;
	}
	*__polarssl_path_buf = path;
#endif
}

bool polarssl_conf::load(void)
{
#ifdef HAS_POLARSSL_DLL
	acl_pthread_once(&__polarssl_once, polarssl_dll_load);
	if (__polarssl_dll == NULL) {
		logger_error("load polarssl error");
		return false;
	}
	return true;
#else
	logger_warn("link polarssl library in static way!");
	return true;
#endif
}

void polarssl_conf::init_once(void)
{
#ifdef HAS_POLARSSL_DLL
	load();
#endif

	lock_.lock();
	if (has_inited_) {
		lock_.unlock();
		return;
	}
#ifdef HAS_POLARSSL
	__entropy_init((entropy_context*) entropy_);
#endif
	has_inited_ = true;
	lock_.unlock();
}

polarssl_conf::polarssl_conf(bool server_side, polarssl_verify_t verify_mode)
{
#ifdef HAS_POLARSSL
	server_side_ = server_side;
	has_inited_  = false;
	entropy_     = acl_mycalloc(1, sizeof(entropy_context));
	cacert_      = NULL;
	cert_chain_  = NULL;
	
	cache_       = NULL;
	pkey_        = NULL;
	verify_mode_ = verify_mode;
#else
	(void) server_side_;
	(void) entropy_;
	(void) cacert_;
	(void) cert_chain_;
	(void) cache_;
	(void) pkey_;
	(void) verify_mode_;
#endif
}

polarssl_conf::~polarssl_conf(void)
{
#ifdef HAS_POLARSSL
	free_ca();

	if (cert_chain_) {
		__x509_free((X509_CRT*) cert_chain_);
		acl_myfree(cert_chain_);
	}

	if (pkey_) {
		__pkey_free((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	if (has_inited_) {
		__entropy_free((entropy_context*) entropy_);
	}
	acl_myfree(entropy_);

	if (cache_) {
		__ssl_cache_free((ssl_cache_context*) cache_);
		acl_myfree(cache_);
	}
#endif
}

void polarssl_conf::free_ca(void)
{
#ifdef HAS_POLARSSL
	if (cacert_) {
		__x509_free((X509_CRT*) cacert_);
		acl_myfree(cacert_);
		cacert_ = NULL;
	}
#endif
}

bool polarssl_conf::load_ca(const char* ca_file, const char* ca_path)
{
#ifdef HAS_POLARSSL
	init_once();

	free_ca();

	int  ret;

	cacert_ = acl_mycalloc(1, sizeof(X509_CRT));
	__x509_init((X509_CRT*) cacert_);

	if (ca_path && *ca_path) {
		ret = __x509_parse_crtpath((X509_CRT*) cacert_, ca_path);
		if (ret != 0) {
			logger_error("x509_crt_parse_path(%s) error: -0x%04x",
				ca_path, -ret);
			free_ca();
			return false;
		}
	}

	if (ca_file == NULL || *ca_file == 0) {
		logger_error("ca_file null");
		free_ca();
		return false;
	}

	ret = __x509_parse_crtfile((X509_CRT*) cacert_, ca_file);
	if (ret != 0) {
		logger_error("x509_crt_parse_path(%s) error: -0x%04x",
			ca_path, -ret);
		free_ca();
		return false;
	} else {
		return true;
	}
#else
	(void) ca_file;
	(void) ca_path;

	logger_error("HAS_POLARSSL not defined!");
	return false;
#endif
}

bool polarssl_conf::add_cert(const char* crt_file)
{
	if (crt_file == NULL || *crt_file == 0) {
		logger_error("crt_file null");
		return false;
	}

#ifdef HAS_POLARSSL
	init_once();

	if (cert_chain_ == NULL) {
		cert_chain_ = acl_mycalloc(1, sizeof(X509_CRT));
		__x509_init((X509_CRT*) cert_chain_);
	}

	int ret = __x509_parse_crtfile((X509_CRT*) cert_chain_, crt_file);
	if (ret != 0) {
		logger_error("x509_crt_parse_file(%s) error: -0x%04x",
			crt_file, -ret);

		__x509_free((X509_CRT*) cert_chain_);
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
	init_once();

	if (pkey_ != NULL) {
		__pkey_free((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	pkey_ = acl_mycalloc(1, sizeof(PKEY));

# ifdef POLARSSL_1_3_X
	__pk_init((PKEY*) pkey_);
# else
	__rsa_init((PKEY*) pkey_, RSA_PKCS_V15, 0);
# endif

	int ret = __x509_parse_keyfile((PKEY*) pkey_, key_file,
			key_pass ? key_pass : "");
	if (ret != 0) {
		logger_error("pk_parse_keyfile(%s) error: -0x%04x",
			key_file, -ret);

		__pkey_free((PKEY*) pkey_);
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
	init_once();

	if (on) {
		if (cache_ != NULL) {
			return;
		}
		cache_ = acl_mycalloc(1, sizeof(ssl_cache_context));
		__ssl_cache_init((ssl_cache_context*) cache_);
	} else if (cache_ != NULL) {
		__ssl_cache_free((ssl_cache_context*) cache_);
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
	init_once();

	ssl_context* ssl = (ssl_context*) ssl_in;

	switch (verify_mode_) {
	case POLARSSL_VERIFY_NONE:
		__ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_NONE);
		break;
	case POLARSSL_VERIFY_OPT:
		__ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_OPTIONAL);
		break;
	case POLARSSL_VERIFY_REQ:
		__ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_REQUIRED);
		break;
	default:
		__ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_NONE);
		break;
	}

	// Setup cipher_suites
	const int* cipher_suites = __ssl_list_ciphersuites();
	if (cipher_suites == NULL) {
		logger_error("ssl_list_ciphersuites null");
		return false;
	}
	__ssl_set_ciphersuites((ssl_context*) ssl, cipher_suites);

//	::ssl_set_min_version((ssl_context*) ssl, SSL_MAJOR_VERSION_3,
//		SSL_MINOR_VERSION_0);
//	::ssl_set_renegotiation((ssl_context*) ssl, SSL_RENEGOTIATION_DISABLED);
//	::ssl_set_dh_param((ssl_context*) &ssl, POLARSSL_DHM_RFC5114_MODP_2048_P,
//		POLARSSL_DHM_RFC5114_MODP_2048_G );

	// Setup cache only for server-side
	if (server_side && cache_ != NULL) {
		__ssl_set_session_cache(ssl, __ssl_cache_get,
			(ssl_cache_context*) cache_, __ssl_cache_set,
			(ssl_cache_context*) cache_);
	}

	// Setup ca cert
	if (cacert_) {
		__ssl_set_ca_chain(ssl, (X509_CRT*) cacert_, NULL, NULL);
	}

	// Setup own's cert chain and private key

	if (cert_chain_ && pkey_) {
# ifdef POLARSSL_1_3_X
		int ret = __ssl_set_own_cert(ssl, (X509_CRT*) cert_chain_,
				(PKEY*) pkey_);
		if (ret != 0) {
			logger_error("ssl_set_own_cert error: -0x%04x", -ret);
			return false;
		}
# else
		__ssl_set_own_cert(ssl, (X509_CRT*) cert_chain_, (PKEY*) pkey_);
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

sslbase_io* polarssl_conf::create(bool nblock)
{
	return NEW polarssl_io(*this, server_side_, nblock);
}

} // namespace acl
