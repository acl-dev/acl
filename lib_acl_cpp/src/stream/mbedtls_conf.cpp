#include "acl_stdafx.hpp"

#ifdef HAS_MBEDTLS_DLL
# ifndef HAS_MBEDTLS
#  define HAS_MBEDTLS
# endif
#endif

#ifdef HAS_MBEDTLS
# include "mbedtls/ssl.h"
# include "mbedtls/entropy.h"
# include "mbedtls/certs.h"
# include "mbedtls/x509_crt.h"
# include "mbedtls/x509.h"
# include "mbedtls/ssl_cache.h"
#endif

#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stdlib/log.hpp"
# include "acl_cpp/stream/mbedtls_io.hpp"
# include "acl_cpp/stream/mbedtls_conf.hpp"
#endif

#if defined(HAS_MBEDTLS)

# define X509_CRT			mbedtls_x509_crt
# define PKEY				mbedtls_pk_context

# ifdef HAS_MBEDTLS_DLL

#  define ENTROPY_FREE_NAME		"mbedtls_entropy_free"
#  define PK_INIT_NAME			"mbedtls_pk_init"
#  define PK_FREE_NAME			"mbedtls_pk_free"
#  define PK_PARSE_KEYFILE_NAME		"mbedtls_pk_parse_keyfile"

#  define X509_INIT_NAME		"mbedtls_x509_crt_init"
#  define X509_FREE_NAME		"mbedtls_x509_crt_free"
#  define X509_PARSE_CRTPATH_NAME	"mbedtls_x509_crt_parse_path"
#  define X509_PARSE_CRTFILE_NAME	"mbedtls_x509_crt_parse_file"

#  define ENTROPY_INIT_NAME		"mbedtls_entropy_init"
#  define SSL_LIST_CIPHERSUITES_NAME	"mbedtls_ssl_list_ciphersuites"
#  define SSL_SET_CIPHERSUITES_NAME	"mbedtls_ssl_conf_ciphersuites"
#  define SSL_SET_SESSION_CACHE_NAME	"mbedtls_ssl_conf_session_cache"
#  define SSL_SET_CA_CHAIN_NAME		"mbedtls_ssl_conf_ca_chain"
#  define SSL_SET_OWN_CERT_NAME		"mbedtls_ssl_conf_own_cert"
#  define SSL_SET_AUTHMODE_NAME		"mbedtls_ssl_conf_authmode"
#  define SSL_CACHE_INIT_NAME		"mbedtls_ssl_cache_init"
#  define SSL_CACHE_FREE_NAME		"mbedtls_ssl_cache_free"
#  define SSL_CACHE_SET_NAME		"mbedtls_ssl_cache_set"
#  define SSL_CACHE_GET_NAME		"mbedtls_ssl_cache_get"

typedef void (*pk_init_fn)(PKEY*);
typedef void (*pk_free_fn)(PKEY*);
typedef int  (*pk_parse_keyfile_fn)(PKEY*, const char*, const char*);

typedef void (*entropy_init_fn)(mbedtls_entropy_context*);
typedef void (*entropy_free_fn)(mbedtls_entropy_context*);

typedef void (*x509_crt_init_fn)(X509_CRT*);
typedef void (*x509_crt_free_fn)(X509_CRT*);
typedef int  (*x509_crt_parse_path_fn)(X509_CRT*, const char*);
typedef int  (*x509_crt_parse_file_fn)(X509_CRT*, const char*);

typedef const int* (*ssl_list_ciphersuites_fn)(void);
typedef void (*ssl_set_ciphersuites_fn)(mbedtls_ssl_context*, const int*);
typedef void (*ssl_set_session_cache_fn)(mbedtls_ssl_context*,
        int (*f_get_cache)(void*, mbedtls_ssl_session*), void*,
        int (*f_set_cache)(void*, const mbedtls_ssl_session*), void*);
typedef void (*ssl_set_ca_chain_fn)(mbedtls_ssl_context*, X509_CRT*,
                       mbedtls_x509_crl*, const char*);
typedef int  (*ssl_set_own_cert_fn)(mbedtls_ssl_context*, X509_CRT*, PKEY*);
typedef void (*ssl_set_authmode_fn)(mbedtls_ssl_context*, int);
typedef void (*ssl_cache_init_fn)(mbedtls_ssl_cache_context*);
typedef void (*ssl_cache_free_fn)(mbedtls_ssl_cache_context*);
typedef int (*ssl_cache_set_fn)(void*, const mbedtls_ssl_session*);;
typedef int (*ssl_cache_get_fn)(void*, mbedtls_ssl_session*);

static entropy_init_fn		__entropy_init;
static entropy_free_fn		__entropy_free;

static pk_init_fn		__pk_init;
static pk_free_fn		__pk_free;
static pk_parse_keyfile_fn	__pk_parse_keyfile;

static x509_crt_init_fn		__x509_init;
static x509_crt_free_fn		__x509_free;
static x509_crt_parse_path_fn	__x509_parse_crtpath;
static x509_crt_parse_file_fn	__x509_parse_crtfile;

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

static acl_pthread_once_t	__mbedtls_once     = ACL_PTHREAD_ONCE_INIT;
static acl::string*		__mbedtls_path_buf = NULL;
#ifdef	ACL_WINDOWS
static const char* __mbedtls_path = "libmbedtls.dll";
#elseif defined(ACL_MACOSX)
static const char* __mbedtls_path = "./libmbedtls.dylib";
#else
static const char* __mbedtls_path = "./libmbedtls.so";
#endif

ACL_DLL_HANDLE			__mbedtls_dll  = NULL;

static void mbedtls_dll_unload(void)
{
	if (__mbedtls_dll) {
		acl_dlclose(__mbedtls_dll);
		__mbedtls_dll = NULL;
		logger("%s unload ok", __mbedtls_path);
	}

	delete __mbedtls_path_buf;
	__mbedtls_path_buf = NULL;
}

extern void mbedtls_dll_load_io(void); // defined in mbedtls_io.cpp

static void mbedtls_dll_load_conf(void)
{
#define LOAD(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__mbedtls_dll, (name));		\
	if ((fn) == NULL)						\
		logger_fatal("dlsym %s error %s", name, acl_dlerror());	\
} while (0)

	LOAD(PK_INIT_NAME, pk_init_fn, __pk_init);
	LOAD(PK_FREE_NAME, pk_free_fn, __pk_free);
	LOAD(PK_PARSE_KEYFILE_NAME, pk_parse_keyfile_fn, __pk_parse_keyfile);

	LOAD(ENTROPY_INIT_NAME, entropy_init_fn, __entropy_init);
	LOAD(ENTROPY_FREE_NAME, entropy_free_fn, __entropy_free);

	LOAD(X509_INIT_NAME, x509_crt_init_fn, __x509_init);
	LOAD(X509_FREE_NAME, x509_crt_free_fn, __x509_free);
	LOAD(X509_PARSE_CRTPATH_NAME, x509_crt_parse_path_fn, __x509_parse_crtpath);
	LOAD(X509_PARSE_CRTFILE_NAME, x509_crt_parse_file_fn, __x509_parse_crtfile);

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
}

static void mbedtls_dll_load(void)
{
	if (__mbedtls_dll) {
		logger("mbedtls(%s) has been loaded!", __mbedtls_path);
		return;
	}

	if (__mbedtls_path_buf != NULL && !__mbedtls_path_buf->empty()) {
		__mbedtls_path = __mbedtls_path_buf->c_str();
	}

	__mbedtls_dll = acl_dlopen(__mbedtls_path);
	if (__mbedtls_dll == NULL) {
		logger_fatal("load %s error %s", __mbedtls_path, acl_dlerror());
	}

	mbedtls_dll_load_conf();
	mbedtls_dll_load_io();

	logger("%s loaded!", __mbedtls_path);
	atexit(mbedtls_dll_unload);
}

# else // !HAS_MBEDTLS_DLL && HAS_MBEDTLS

#  define __entropy_free	::entropy_free
#  define __pk_init		::pk_init
#  define __pk_free		::pk_free
#  define __pk_parse_keyfile	::pk_parse_keyfile
#  define __x509_init		::x509_crt_init
#  define __x509_free		::x509_crt_free
#  define __x509_parse_crtpath	::x509_crt_parse_path
#  define __x509_parse_crtfile	::x509_crt_parse_file

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

# endif // HAS_MBEDTLS_DLL
#endif  // HAS_MBEDTLS

namespace acl
{

void mbedtls_conf::set_libpath(const char* path acl_unused)
{
#ifdef HAS_MBEDTLS_DLL
	if (__mbedtls_path_buf == NULL) {
		__mbedtls_path_buf = NEW string;
	}
	*__mbedtls_path_buf = path;
#endif
}

void mbedtls_conf::load(void)
{
#ifdef HAS_MBEDTLS_DLL
	acl_pthread_once(&__mbedtls_once, mbedtls_dll_load);
#else
	logger_warn("link mbedtls library in statis way!");
#endif
}

void mbedtls_conf::init_once(void)
{
	load();

	lock_.lock();
	if (has_inited_) {
		lock_.unlock();
		return;
	}
#ifdef HAS_MBEDTLS
	__entropy_init((mbedtls_entropy_context*) entropy_);
#endif
	has_inited_ = true;
	lock_.unlock();
}

mbedtls_conf::mbedtls_conf(void)
{
#ifdef HAS_MBEDTLS
	has_inited_  = false;
	entropy_     = acl_mycalloc(1, sizeof(mbedtls_entropy_context));
	cacert_      = NULL;
	cert_chain_  = NULL;
	
	cache_       = NULL;
	pkey_        = NULL;
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

mbedtls_conf::~mbedtls_conf(void)
{
#ifdef HAS_MBEDTLS
	free_ca();

	if (cert_chain_) {
		__x509_free((X509_CRT*) cert_chain_);
		acl_myfree(cert_chain_);
	}

	if (pkey_) {
		__pk_free((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	if (has_inited_) {
		__entropy_free((mbedtls_entropy_context*) entropy_);
	}
	acl_myfree(entropy_);

	if (cache_) {
		__ssl_cache_free((mbedtls_ssl_cache_context*) cache_);
		acl_myfree(cache_);
	}
#endif
}

void mbedtls_conf::free_ca(void)
{
#ifdef HAS_MBEDTLS
	if (cacert_) {
		__x509_free((X509_CRT*) cacert_);
		acl_myfree(cacert_);
		cacert_ = NULL;
	}
#endif
}

bool mbedtls_conf::load_ca(const char* ca_file, const char* ca_path)
{
#ifdef HAS_MBEDTLS
	init_once();

	free_ca();

	int  ret;

	cacert_ = acl_mycalloc(1, sizeof(X509_CRT));
	__x509_init((X509_CRT*) cacert_);

	if (ca_path && *ca_path) {
		ret = __x509_parse_crtpath((X509_CRT*) cacert_, ca_path);
		if (ret != 0) {
			logger_error("x509_crt_parse_path(%s) error: -0x%04x",
				ca_path, ret);
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
			ca_path, ret);
		free_ca();
		return false;
	} else {
		return true;
	}
#else
	(void) ca_file;
	(void) ca_path;

	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

bool mbedtls_conf::add_cert(const char* crt_file)
{
	if (crt_file == NULL || *crt_file == 0) {
		logger_error("crt_file null");
		return false;
	}

#ifdef HAS_MBEDTLS
	init_once();

	if (cert_chain_ == NULL) {
		cert_chain_ = acl_mycalloc(1, sizeof(X509_CRT));
		__x509_init((X509_CRT*) cert_chain_);
	}

	int ret = __x509_parse_crtfile((X509_CRT*) cert_chain_, crt_file);
	if (ret != 0) {
		logger_error("x509_crt_parse_file(%s) error: -0x%04x",
			crt_file, ret);

		__x509_free((X509_CRT*) cert_chain_);
		acl_myfree(cert_chain_);
		cert_chain_ = NULL;
		return false;
	}

	return true;
#else
	(void) crt_file;
	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

bool mbedtls_conf::set_key(const char* key_file,
	const char* key_pass /* = NULL */)
{
#ifdef HAS_MBEDTLS
	init_once();

	if (pkey_ != NULL) {
		__pk_free((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	pkey_ = acl_mycalloc(1, sizeof(PKEY));

	__pk_init((PKEY*) pkey_);

	int ret = __pk_parse_keyfile((PKEY*) pkey_, key_file,
			key_pass ? key_pass : "");
	if (ret != 0) {
		logger_error("pk_parse_keyfile(%s) error: -0x%04x",
			key_file, ret);

		__pk_free((PKEY*) pkey_);
		acl_myfree(pkey_);
		pkey_ = NULL;
		return false;
	}

	return true;
#else
	(void) key_file;
	(void) key_pass;

	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

void mbedtls_conf::enable_cache(bool on)
{
#ifdef HAS_MBEDTLS
	init_once();

	if (on) {
		if (cache_ != NULL) {
			return;
		}
		cache_ = acl_mycalloc(1, sizeof(mbedtls_ssl_cache_context));
		__ssl_cache_init((mbedtls_ssl_cache_context*) cache_);
	} else if (cache_ != NULL) {
		__ssl_cache_free((mbedtls_ssl_cache_context*) cache_);
		acl_myfree(cache_);
		cache_ = NULL;
	}
#else
	(void) on;
	logger_error("HAS_MBEDTLS not defined!");
#endif
}

bool mbedtls_conf::setup_certs(void* ssl_in, bool server_side)
{
#ifdef HAS_MBEDTLS
	init_once();

	mbedtls_ssl_context* ssl = (mbedtls_ssl_context*) ssl_in;

	switch (verify_mode_) {
	case POLARSSL_VERIFY_NONE:
		__ssl_set_authmode((mbedtls_ssl_context*) ssl, MBEDTLS_SSL_VERIFY_NONE);
		break;
	case POLARSSL_VERIFY_OPT:
		__ssl_set_authmode((mbedtls_ssl_context*) ssl, MBEDTLS_SSL_VERIFY_OPTIONAL);
		break;
	case POLARSSL_VERIFY_REQ:
		__ssl_set_authmode((mbedtls_ssl_context*) ssl, MBEDTLS_SSL_VERIFY_REQUIRED);
		break;
	default:
		__ssl_set_authmode((mbedtls_ssl_context*) ssl, MBEDTLS_SSL_VERIFY_NONE);
		break;
	}

	// Setup cipher_suites
	const int* cipher_suites = __ssl_list_ciphersuites();
	if (cipher_suites == NULL) {
		logger_error("ssl_list_ciphersuites null");
		return false;
	}
	__ssl_set_ciphersuites((mbedtls_ssl_context*) ssl, cipher_suites);

//	::ssl_set_min_version((mbedtls_ssl_context*) ssl, MBEDTLS_SSL_MAJOR_VERSION_3,
//		SSL_MINOR_VERSION_0);
//	::ssl_set_renegotiation((mbedtls_ssl_context*) ssl, MBEDTLS_SSL_RENEGOTIATION_DISABLED);
//	::ssl_set_dh_param((mbedtls_ssl_context*) &ssl, MBEDTLS_DHM_RFC5114_MODP_2048_P,
//		MBEDTLS_DHM_RFC5114_MODP_2048_G );

	// Setup cache only for server-side
	if (server_side && cache_ != NULL) {
		__ssl_set_session_cache(ssl, __ssl_cache_get,
			(mbedtls_ssl_cache_context*) cache_, __ssl_cache_set,
			(mbedtls_ssl_cache_context*) cache_);
	}

	// Setup ca cert
	if (cacert_) {
		__ssl_set_ca_chain(ssl, (X509_CRT*) cacert_, NULL, NULL);
	}

	// Setup own's cert chain and private key

	if (cert_chain_ && pkey_) {
		int ret = __ssl_set_own_cert(ssl, (X509_CRT*) cert_chain_,
				(PKEY*) pkey_);
		if (ret != 0) {
			logger_error("ssl_set_own_cert error: -0x%04x", ret);
			return false;
		}
	}

	return true;
#else
	(void) ssl_in;
	(void) server_side;
	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

polarssl_io* mbedtls_conf::create_io(bool server_side, bool nblock)
{
	return new mbedtls_io(*this, server_side, nblock);
}

} // namespace acl
