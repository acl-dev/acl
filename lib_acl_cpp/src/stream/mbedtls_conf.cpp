#include "acl_stdafx.hpp"

#ifdef HAS_MBEDTLS_DLL
# ifndef HAS_MBEDTLS
#  define HAS_MBEDTLS
# endif
#endif

//#define DEBUG_SSL

#ifdef HAS_MBEDTLS
# include "mbedtls/version.h"
# if MBEDTLS_VERSION_MAJOR==2 && MBEDTLS_VERSION_MINOR==7 && MBEDTLS_VERSION_PATCH==12
#  include "mbedtls/2.7.12/threading_alt.h"
#  include "mbedtls/2.7.12/ssl.h"
#  include "mbedtls/2.7.12/ssl_internal.h" // for mbedtls_x509_crt, mbedtls_pk_context,
#  include "mbedtls/2.7.12/error.h"
#  include "mbedtls/2.7.12/ctr_drbg.h"
#  include "mbedtls/2.7.12/entropy.h"
#  include "mbedtls/2.7.12/certs.h"
#  include "mbedtls/2.7.12/x509_crt.h"
#  include "mbedtls/2.7.12/x509.h"
#  include "mbedtls/2.7.12/ssl_cache.h"
# elif MBEDTLS_VERSION_MAJOR==3 && MBEDTLS_VERSION_MINOR==3 && MBEDTLS_VERSION_PATCH==0
#  include "mbedtls/3.3.0/mbedtls/ssl.h"
#  include "mbedtls/3.3.0/mbedtls/error.h"
#  include "mbedtls/3.3.0/mbedtls/ctr_drbg.h"
#  include "mbedtls/3.3.0/mbedtls/entropy.h"
#  include "mbedtls/3.3.0/mbedtls/x509_crt.h"
#  include "mbedtls/3.3.0/mbedtls/x509.h"
#  include "mbedtls/3.3.0/mbedtls/ssl_cache.h"
# else
#  error "Unsupport the current version"
# endif
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

#  ifdef MBEDTLS_THREADING_ALT
#   define THREADING_SET_ALT_NAME	"mbedtls_threading_set_alt"
#  endif
#  define PK_INIT_NAME			"mbedtls_pk_init"
#  define PK_FREE_NAME			"mbedtls_pk_free"
#  define PK_PARSE_KEYFILE_NAME		"mbedtls_pk_parse_keyfile"

#  ifdef HAS_HAVEGE
#   define HAVEGE_INIT_NAME		"mbedtls_havege_init"
#   define HAVEGE_RANDOM_NAME		"mbedtls_havege_random"
#  else
#   define CTR_DRBG_INIT_NAME		"mbedtls_ctr_drbg_init"
#   define CTR_DRBG_FREE_NAME		"mbedtls_ctr_drbg_free"
#   define CTR_DRBG_SEED_NAME		"mbedtls_ctr_drbg_seed"
#   define CTR_DRBG_RANDOM_NAME		"mbedtls_ctr_drbg_random"
#  endif

#  define ENTROPY_INIT_NAME		"mbedtls_entropy_init"
#  define ENTROPY_FREE_NAME		"mbedtls_entropy_free"
#  define ENTROPY_FUNC_NAME		"mbedtls_entropy_func"

#  define X509_INIT_NAME		"mbedtls_x509_crt_init"
#  define X509_FREE_NAME		"mbedtls_x509_crt_free"
#  define X509_PARSE_CRT_PATH_NAME	"mbedtls_x509_crt_parse_path"
#  define X509_PARSE_CRT_FILE_NAME	"mbedtls_x509_crt_parse_file"

#  define SSL_LIST_CIPHERSUITES_NAME	"mbedtls_ssl_list_ciphersuites"

#  define SSL_CONF_INIT_NAME		"mbedtls_ssl_config_init"
#  define SSL_CONF_FREE_NAME		"mbedtls_ssl_config_free"
#  define SSL_CONF_DEFAULTS_NAME	"mbedtls_ssl_config_defaults"
#  define SSL_CONF_RNG_NAME		"mbedtls_ssl_conf_rng"
#  define SSL_CONF_ENDPOINT_NAME	"mbedtls_ssl_conf_endpoint"
#  define SSL_CONF_CIPHERSUITES_NAME	"mbedtls_ssl_conf_ciphersuites"
#  define SSL_CONF_SESSION_CACHE_NAME	"mbedtls_ssl_conf_session_cache"
#  define SSL_CONF_CA_CHAIN_NAME	"mbedtls_ssl_conf_ca_chain"
#  define SSL_CONF_OWN_CERT_NAME	"mbedtls_ssl_conf_own_cert"
#  define SSL_CONF_AUTHMODE_NAME	"mbedtls_ssl_conf_authmode"
#  define SSL_CONF_SNI_NAME		"mbedtls_ssl_conf_sni"
#  ifdef DEBUG_SSL
#   define SSL_CONF_DBG_NAME		"mbedtls_ssl_conf_dbg"
#  endif

#  define SSL_CACHE_INIT_NAME		"mbedtls_ssl_cache_init"
#  define SSL_CACHE_FREE_NAME		"mbedtls_ssl_cache_free"
#  define SSL_CACHE_SET_NAME		"mbedtls_ssl_cache_set"
#  define SSL_CACHE_GET_NAME		"mbedtls_ssl_cache_get"

#  define SSL_SETUP_NAME		"mbedtls_ssl_setup"
#  define SSL_SET_CERT_NAME		"mbedtls_ssl_set_hs_own_cert"

#  ifdef MBEDTLS_THREADING_ALT
typedef void (*threading_set_alt_fn)(
	void (*init)(mbedtls_threading_mutex_t *),
	void (*free)(mbedtls_threading_mutex_t *),
	int (*lock)(mbedtls_threading_mutex_t *),
	int (*unlock)( mbedtls_threading_mutex_t *));
#  endif // MBEDTLS_THREADING_ALT

typedef void (*pk_init_fn)(PKEY*);
typedef void (*pk_free_fn)(PKEY*);
typedef int  (*pk_parse_keyfile_fn)(PKEY*, const char*, const char*);

# ifdef HAS_HAVEGE
typedef void (*havege_init_fn)(mbedtls_havege_state*);
typedef int  (*havege_random_fn)(void*, unsigned char*, size_t);
# else
typedef void (*ctr_drbg_init_fn)(mbedtls_ctr_drbg_context*);
typedef void (*ctr_drbg_free_fn)(mbedtls_ctr_drbg_context*);
typedef int  (*ctr_drbg_seed_fn)(mbedtls_ctr_drbg_context*,
		int (*)(void*, unsigned char*, size_t), void *,
		const unsigned char*, size_t);
typedef int  (*ctr_drbg_random_fn)(void*, unsigned char*, size_t);
# endif

typedef void (*entropy_init_fn)(mbedtls_entropy_context*);
typedef void (*entropy_free_fn)(mbedtls_entropy_context*);
typedef int  (*entropy_func_fn)(void*, unsigned char*, size_t);

typedef void (*x509_crt_init_fn)(X509_CRT*);
typedef void (*x509_crt_free_fn)(X509_CRT*);
typedef int  (*x509_crt_parse_path_fn)(X509_CRT*, const char*);
typedef int  (*x509_crt_parse_file_fn)(X509_CRT*, const char*);

typedef const int* (*ssl_list_ciphersuites_fn)(void);

typedef void (*ssl_config_init_fn)(mbedtls_ssl_config*);
typedef void (*ssl_config_free_fn)(mbedtls_ssl_config*);
typedef void (*ssl_conf_rng_fn)(mbedtls_ssl_config*,
		int (*f_rng)(void*, unsigned char*, size_t), void *p_rng);
typedef void (*ssl_conf_endpoint_fn)(mbedtls_ssl_config*, int);
typedef void (*ssl_conf_ciphersuites_fn)(mbedtls_ssl_config*, const int*);
typedef void (*ssl_conf_session_cache_fn)(mbedtls_ssl_config*, void*,
        int (*)(void*, mbedtls_ssl_session*),
        int (*)(void*, const mbedtls_ssl_session*));
typedef void (*ssl_conf_ca_chain_fn)(mbedtls_ssl_config*, X509_CRT*,
                       mbedtls_x509_crl*);
typedef int  (*ssl_conf_own_cert_fn)(mbedtls_ssl_config*, X509_CRT*, PKEY*);
typedef void (*ssl_conf_authmode_fn)(mbedtls_ssl_config*, int);

typedef void (*ssl_conf_sni_fn)(mbedtls_ssl_config*,
		int (*f_sni)(void*, mbedtls_ssl_context*,
			const unsigned char*,
			size_t),
		void*);

# ifdef DEBUG_SSL
typedef void (*ssl_conf_dbg_fn)(mbedtls_ssl_config*,
		void (*)(void*, int, const char*, int , const char*), void*);
# endif
typedef int  (*ssl_config_defaults_fn)(mbedtls_ssl_config*, int, int, int);
typedef void (*ssl_cache_init_fn)(mbedtls_ssl_cache_context*);
typedef void (*ssl_cache_free_fn)(mbedtls_ssl_cache_context*);
typedef int  (*ssl_cache_set_fn)(void*, const mbedtls_ssl_session*);;
typedef int  (*ssl_cache_get_fn)(void*, mbedtls_ssl_session*);

typedef int  (*ssl_setup_fn)(mbedtls_ssl_context*, mbedtls_ssl_config*);
typedef int  (*ssl_set_cert_fn)(mbedtls_ssl_context*,
		mbedtls_x509_crt*, mbedtls_pk_context*);

# ifdef MBEDTLS_THREADING_ALT
static threading_set_alt_fn		__threading_set_alt;
# endif

static pk_init_fn			__pk_init;
static pk_free_fn			__pk_free;
static pk_parse_keyfile_fn		__pk_parse_keyfile;

# ifdef HAS_HAVEGE
static havege_init_fn			__havege_init;
static havege_random_fn			__havege_random;
# else
static ctr_drbg_init_fn			__ctr_drbg_init;
static ctr_drbg_free_fn			__ctr_drbg_free;
static ctr_drbg_seed_fn			__ctr_drbg_seed;
static ctr_drbg_random_fn		__ctr_drbg_random;
# endif

static entropy_init_fn			__entropy_init;
static entropy_free_fn			__entropy_free;
static entropy_func_fn			__entropy_func;

static x509_crt_init_fn			__x509_crt_init;
static x509_crt_free_fn			__x509_crt_free;
static x509_crt_parse_path_fn		__x509_crt_parse_path;
static x509_crt_parse_file_fn		__x509_crt_parse_file;

static ssl_list_ciphersuites_fn		__ssl_list_ciphersuites;

static ssl_config_init_fn		__ssl_config_init;
static ssl_config_free_fn		__ssl_config_free;
static ssl_config_defaults_fn		__ssl_config_defaults;
static ssl_conf_rng_fn			__ssl_conf_rng;
static ssl_conf_endpoint_fn		__ssl_conf_endpoint;
static ssl_conf_ciphersuites_fn		__ssl_conf_ciphersuites;
static ssl_conf_session_cache_fn	__ssl_conf_session_cache;
static ssl_conf_ca_chain_fn		__ssl_conf_ca_chain;
static ssl_conf_own_cert_fn		__ssl_conf_own_cert;
static ssl_conf_authmode_fn		__ssl_conf_authmode;
static ssl_conf_sni_fn			__ssl_conf_sni;
# ifdef DEBUG_SSL
static ssl_conf_dbg_fn			__ssl_conf_dbg;
# endif

static ssl_cache_init_fn		__ssl_cache_init;
static ssl_cache_free_fn		__ssl_cache_free;
static ssl_cache_set_fn			__ssl_cache_set;
static ssl_cache_get_fn			__ssl_cache_get;

static ssl_setup_fn			__ssl_setup;
static ssl_set_cert_fn			__ssl_set_cert;

static acl_pthread_once_t __load_once = ACL_PTHREAD_ONCE_INIT;
static acl::string* __crypto_path_buf  = NULL;
static acl::string* __x509_path_buf    = NULL;
static acl::string* __tls_path_buf     = NULL;

# if defined(_WIN32) || defined(_WIN64)
static const char* __crypto_path       = "libmbedcrypto.dll";
static const char* __x509_path         = "libmbedx509.dll";
static const char* __tls_path          = "libmbedtls.dll";
# elif defined(ACL_MACOSX)
static const char* __crypto_path       = "./libmbedcrypto.dylib";
static const char* __x509_path         = "./libmbedx509.dylib";
static const char* __tls_path          = "./libmbedtls_all.dylib";
# else
static const char* __crypto_path       = "./libmbedcrypto.so";
static const char* __x509_path         = "./libmbedx509.so";
static const char* __tls_path          = "./libmbedtls_all.so";
# endif

ACL_DLL_HANDLE __crypto_dll            = NULL;
ACL_DLL_HANDLE __x509_dll              = NULL;
ACL_DLL_HANDLE __tls_dll               = NULL;

#ifndef HAVE_NO_ATEXIT
static void mbedtls_dll_unload(void)
{
	if (__crypto_dll) {
		acl_dlclose(__crypto_dll);
		//logger("%s unload ok", __crypto_path);
	}
	if (__x509_dll && __x509_dll != __crypto_dll) {
		acl_dlclose(__x509_dll);
		//logger("%s unload ok", __x509_path);
	}
	if (__tls_dll && __tls_dll != __crypto_dll) {
		acl_dlclose(__tls_dll);
		//logger("%s unload ok", __tls_path);
	}

	__crypto_dll = NULL;
	__x509_dll = NULL;
	__tls_dll = NULL;

	delete __crypto_path_buf;
	delete __x509_path_buf;
	delete __tls_path_buf;

	__crypto_path_buf = NULL;
	__x509_path_buf   = NULL;
	__tls_path_buf    = NULL;
}
#endif

extern bool mbedtls_load_io(void); // defined in mbedtls_io.cpp

#define LOAD_CRYPTO(name, type, fn) do {				\
	(fn) = (type) acl_dlsym(__crypto_dll, (name));			\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s, lib=%s",		\
			name, acl_dlerror(), __crypto_path);		\
		return false;						\
	}								\
} while (0)

#define LOAD_X509(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__x509_dll, (name));			\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s, lib=%s",		\
			name, acl_dlerror(), __x509_path);		\
		return false;						\
	}								\
} while (0)

#define LOAD_SSL(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__tls_dll, (name));			\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s, lib=%s",		\
			name, acl_dlerror(), __tls_path);		\
		return false;						\
	}								\
} while (0)

static bool load_from_crypto(void)
{
# ifdef MBEDTLS_THREADING_ALT
	LOAD_CRYPTO(THREADING_SET_ALT_NAME, threading_set_alt_fn, __threading_set_alt);
#endif
	LOAD_CRYPTO(PK_INIT_NAME, pk_init_fn, __pk_init);
	LOAD_CRYPTO(PK_FREE_NAME, pk_free_fn, __pk_free);
	LOAD_CRYPTO(PK_PARSE_KEYFILE_NAME, pk_parse_keyfile_fn, __pk_parse_keyfile);

# ifdef HAS_HAVEGE
	LOAD_CRYPTO(HAVEGE_INIT_NAME, havege_init_fn, __havege_init);
	LOAD_CRYPTO(HAVEGE_RANDOM_NAME, havege_random_fn, __havege_random);
# else
	LOAD_CRYPTO(CTR_DRBG_INIT_NAME, ctr_drbg_init_fn, __ctr_drbg_init);
	LOAD_CRYPTO(CTR_DRBG_FREE_NAME, ctr_drbg_free_fn, __ctr_drbg_free);
	LOAD_CRYPTO(CTR_DRBG_SEED_NAME, ctr_drbg_seed_fn, __ctr_drbg_seed);
	LOAD_CRYPTO(CTR_DRBG_RANDOM_NAME, ctr_drbg_random_fn, __ctr_drbg_random);
# endif

	LOAD_CRYPTO(ENTROPY_INIT_NAME, entropy_init_fn, __entropy_init);
	LOAD_CRYPTO(ENTROPY_FREE_NAME, entropy_free_fn, __entropy_free);
	LOAD_CRYPTO(ENTROPY_FUNC_NAME, entropy_func_fn, __entropy_func);

	return true;
}

static bool load_from_x509(void)
{
	LOAD_X509(X509_INIT_NAME, x509_crt_init_fn, __x509_crt_init);
	LOAD_X509(X509_FREE_NAME, x509_crt_free_fn, __x509_crt_free);
	LOAD_X509(X509_PARSE_CRT_PATH_NAME, x509_crt_parse_path_fn, __x509_crt_parse_path);
	LOAD_X509(X509_PARSE_CRT_FILE_NAME, x509_crt_parse_file_fn, __x509_crt_parse_file);

	return true;
}

static bool load_from_ssl(void)
{
	LOAD_SSL(SSL_LIST_CIPHERSUITES_NAME, ssl_list_ciphersuites_fn, __ssl_list_ciphersuites);

	LOAD_SSL(SSL_CONF_INIT_NAME, ssl_config_init_fn, __ssl_config_init);
	LOAD_SSL(SSL_CONF_FREE_NAME, ssl_config_free_fn, __ssl_config_free);
	LOAD_SSL(SSL_CONF_DEFAULTS_NAME, ssl_config_defaults_fn, __ssl_config_defaults);
	LOAD_SSL(SSL_CONF_RNG_NAME, ssl_conf_rng_fn, __ssl_conf_rng);
	LOAD_SSL(SSL_CONF_ENDPOINT_NAME, ssl_conf_endpoint_fn, __ssl_conf_endpoint);
	LOAD_SSL(SSL_CONF_CIPHERSUITES_NAME, ssl_conf_ciphersuites_fn, __ssl_conf_ciphersuites);
	LOAD_SSL(SSL_CONF_SESSION_CACHE_NAME, ssl_conf_session_cache_fn, __ssl_conf_session_cache);
	LOAD_SSL(SSL_CONF_CA_CHAIN_NAME, ssl_conf_ca_chain_fn, __ssl_conf_ca_chain);
	LOAD_SSL(SSL_CONF_OWN_CERT_NAME, ssl_conf_own_cert_fn, __ssl_conf_own_cert);
	LOAD_SSL(SSL_CONF_AUTHMODE_NAME, ssl_conf_authmode_fn, __ssl_conf_authmode);
	LOAD_SSL(SSL_CONF_SNI_NAME, ssl_conf_sni_fn, __ssl_conf_sni);
# ifdef DEBUG_SSL
	LOAD_SSL(SSL_CONF_DBG_NAME, ssl_conf_dbg_fn, __ssl_conf_dbg);
# endif

	LOAD_SSL(SSL_CACHE_INIT_NAME, ssl_cache_init_fn, __ssl_cache_init);
	LOAD_SSL(SSL_CACHE_FREE_NAME, ssl_cache_free_fn, __ssl_cache_free);
	LOAD_SSL(SSL_CACHE_SET_NAME, ssl_cache_set_fn, __ssl_cache_set);
	LOAD_SSL(SSL_CACHE_GET_NAME, ssl_cache_get_fn, __ssl_cache_get);

	LOAD_SSL(SSL_SETUP_NAME, ssl_setup_fn, __ssl_setup);
	LOAD_SSL(SSL_SET_CERT_NAME, ssl_set_cert_fn, __ssl_set_cert);
	return true;
}

static bool load_all_dlls(void)
{
	if (__crypto_path_buf && !__crypto_path_buf->empty()) {
		__crypto_path = __crypto_path_buf->c_str();
	}
	if (__x509_path_buf && !__x509_path_buf->empty()) {
		__x509_path = __x509_path_buf->c_str();
	}
	if (__tls_path_buf && !__tls_path_buf->empty()) {
		__tls_path = __tls_path_buf->c_str();
	}

	__crypto_dll = acl_dlopen(__crypto_path);
	if (__crypto_dll == NULL) {
		logger_error("load %s error %s", __crypto_path, acl_dlerror());
		return false;
	}

	if (strcmp(__x509_path, __crypto_path) == 0) {
		__x509_dll = __crypto_dll;
	} else if ((__x509_dll = acl_dlopen(__x509_path)) == NULL) {
		logger_error("load %s error %s", __x509_path, acl_dlerror());
		return false;
	}

	if (strcmp(__tls_path, __crypto_path) == 0) {
		__tls_dll = __crypto_dll;
	} else if ((__tls_dll = acl_dlopen(__tls_path)) == NULL) {
		logger_error("load %s error %s", __tls_path, acl_dlerror());
		return false;
	}
	return true;
}

static bool mbedtls_load_conf(void)
{
	if (!load_from_crypto()) {
		return false;
	}
	if (!load_from_x509()) {
		return false;
	}
	return load_from_ssl();
}

static void mbedtls_dll_load(void)
{
	if (__tls_dll) {
		logger("mbedtls(%s) has been loaded!", __tls_path);
		return;
	}

	if (!load_all_dlls()) {
		return;
	}

	if (!mbedtls_load_conf()) {
		logger_error("mbedtls_dll_load_conf %s error", __tls_path);
		acl_dlclose(__tls_dll);
		__tls_dll = NULL;
		return;
	}

	if (!mbedtls_load_io()) {
		logger_error("mbedtls_dll_load_io %s error", __tls_path);
		acl_dlclose(__tls_dll);
		__tls_dll = NULL;
		return;
	}

	logger("%s loaded!", __crypto_path);
	if (strcmp(__crypto_path, __x509_path) != 0) {
		logger("%s loaded!", __x509_path);
	}
	if (strcmp(__crypto_path, __tls_path) != 0) {
		logger("%s loaded!", __tls_path);
	}
#ifndef HAVE_NO_ATEXIT
	atexit(mbedtls_dll_unload);
#endif
}

# else // !HAS_MBEDTLS_DLL && HAS_MBEDTLS

#  ifdef MBEDTLS_THREADING_ALT
#   define __threading_set_alt		::mbedtls_threading_set_alt
#  endif // MBEDTLS_THREADING_ALT

#  define __pk_init			::mbedtls_pk_init
#  define __pk_free			::mbedtls_pk_free
#  define __pk_parse_keyfile		::mbedtls_pk_parse_keyfile
#  ifdef HAS_HAVEGE
#   define __havege_init		::mbedtls_havege_init
#   define __havege_random		::mbedtls_havege_random
#  else
#   define __ctr_drbg_init		::mbedtls_ctr_drbg_init
#   define __ctr_drbg_free		::mbedtls_ctr_drbg_free
#   define __ctr_drbg_seed		::mbedtls_ctr_drbg_seed
#   define __ctr_drbg_random		::mbedtls_ctr_drbg_random
#  endif
#  define __entropy_init		::mbedtls_entropy_init
#  define __entropy_free		::mbedtls_entropy_free
#  define __entropy_func		::mbedtls_entropy_func

#  define __x509_crt_init		::mbedtls_x509_crt_init
#  define __x509_crt_free		::mbedtls_x509_crt_free
#  define __x509_crt_parse_path		::mbedtls_x509_crt_parse_path
#  define __x509_crt_parse_file		::mbedtls_x509_crt_parse_file

#  define __ssl_list_ciphersuites	::mbedtls_ssl_list_ciphersuites
#  define __ssl_config_init		::mbedtls_ssl_config_init
#  define __ssl_config_free		::mbedtls_ssl_config_free
#  define __ssl_config_defaults		::mbedtls_ssl_config_defaults
#  define __ssl_conf_rng		::mbedtls_ssl_conf_rng
#  define __ssl_conf_endpoint		::mbedtls_ssl_conf_endpoint
#  define __ssl_conf_ciphersuites	::mbedtls_ssl_conf_ciphersuites
#  define __ssl_conf_session_cache	::mbedtls_ssl_conf_session_cache
#  define __ssl_conf_ca_chain		::mbedtls_ssl_conf_ca_chain
#  define __ssl_conf_own_cert		::mbedtls_ssl_conf_own_cert
#  define __ssl_conf_authmode		::mbedtls_ssl_conf_authmode
#  define __ssl_conf_sni		::mbedtls_ssl_conf_sni
#  ifdef DEBUG_SSL
#   define __ssl_conf_dbg		::mbedtls_ssl_conf_dbg
#  endif
#  define __ssl_cache_init		::mbedtls_ssl_cache_init
#  define __ssl_cache_free		::mbedtls_ssl_cache_free
#  define __ssl_cache_set		::mbedtls_ssl_cache_set
#  define __ssl_cache_get		::mbedtls_ssl_cache_get
#  define __ssl_setup			::mbedtls_ssl_setup
#  define __ssl_set_cert		::mbedtls_ssl_set_hs_own_cert

# endif // HAS_MBEDTLS_DLL
#endif  // HAS_MBEDTLS

namespace acl {

#define CONF_INIT_NIL	0
#define CONF_INIT_OK	1
#define CONF_INIT_ERR	2

void mbedtls_conf::set_libpath(const char* libmbedcrypto,
	const char* libmbedx509, const char* libmbedtls)
{
#ifdef HAS_MBEDTLS_DLL
	if (__crypto_path_buf == NULL) {
		__crypto_path_buf = NEW string;
	}
	*__crypto_path_buf = libmbedcrypto;

	if (__x509_path_buf == NULL) {
		__x509_path_buf = NEW string;
	}
	*__x509_path_buf = libmbedx509;

	if (__tls_path_buf == NULL) {
		__tls_path_buf = NEW string;
	}
	*__tls_path_buf = libmbedtls;
#else
	(void) libmbedcrypto;
	(void) libmbedx509;
	(void) libmbedtls;
#endif
}

void mbedtls_conf::set_libpath(const char* libmbedtls)
{
	set_libpath(libmbedtls, libmbedtls, libmbedtls);
}

bool mbedtls_conf::load(void)
{
#ifdef HAS_MBEDTLS_DLL
	acl_pthread_once(&__load_once, mbedtls_dll_load);
	if (__crypto_dll == NULL || __x509_dll == NULL || __tls_dll == NULL) {
		logger_error("load mbedtls error");
		return false;
	}
	return true;
#else
	logger_warn("link mbedtls library in static way!");
	return true;
#endif
}

#if defined(HAS_MBEDTLS) && defined(MBEDTLS_THREADING_ALT)
static void mutex_init(mbedtls_threading_mutex_t* mutex)
{
	acl_pthread_mutex_t* m = (acl_pthread_mutex_t*) mutex;
	acl_pthread_mutex_init(m, NULL);
}

static void mutex_free(mbedtls_threading_mutex_t* mutex)
{
	acl_pthread_mutex_t* m = (acl_pthread_mutex_t*) mutex;
	acl_pthread_mutex_destroy(m);
}

static int mutex_lock(mbedtls_threading_mutex_t* mutex)
{
	acl_pthread_mutex_t* m = (acl_pthread_mutex_t*) mutex;
	return acl_pthread_mutex_lock(m);
}

static int mutex_unlock(mbedtls_threading_mutex_t* mutex)
{
	acl_pthread_mutex_t* m = (acl_pthread_mutex_t*) mutex;
	return acl_pthread_mutex_unlock(m);
}
#endif

#ifdef HAS_MBEDTLS
static int __once_inited = CONF_INIT_NIL;

static void mbedtls_once(void)
{
	if (!mbedtls_conf::load()) {
		__once_inited = CONF_INIT_ERR;
		return;
	}
# if defined(MBEDTLS_THREADING_ALT)
	__threading_set_alt(mutex_init, mutex_free, mutex_lock, mutex_unlock);
# endif
	__once_inited = CONF_INIT_OK;
}
#endif // HAS_MBEDTLS

#ifdef HAS_MBEDTLS
static void set_authmode(mbedtls_ssl_config* conf, mbedtls_verify_t verify_mode)
{
	switch (verify_mode) {
	case MBEDTLS_VERIFY_NONE:
		__ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_NONE);
		break;
	case MBEDTLS_VERIFY_OPT:
		__ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
		break;
	case MBEDTLS_VERIFY_REQ:
		__ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_REQUIRED);
		break;
	default:
		__ssl_conf_authmode(conf, MBEDTLS_SSL_VERIFY_NONE);
		break;
	}
}
#endif

bool mbedtls_conf::init_rand(void)
{
#ifdef HAS_MBEDTLS
	char pers[50];
	safe_snprintf(pers, sizeof(pers), "SSL Pthread Thread %lu",
		(unsigned long) acl_pthread_self());
	int ret = __ctr_drbg_seed((mbedtls_ctr_drbg_context*) rnd_,
			__entropy_func,
			(mbedtls_entropy_context*) entropy_,
			(const unsigned char *) pers, strlen(pers));
	if (ret != 0) {
		logger_error("ctr_drbg_init error: -0x%04x\n", -ret);
		return false;
	}

	return true;
#else
	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

#ifdef HAS_MBEDTLS
# ifdef DEBUG_SSL
static void my_debug( void *ctx, int level, const char* fname, int line,
	const char* str)
{
	fprintf((FILE *) ctx, "%s(%d): level=%d, %s", fname, line, level, str);
	fflush((FILE *) ctx);
}
# endif
#endif

int mbedtls_conf::sni_callback(void* arg, mbedtls_ssl_context* ssl,
	const unsigned char* name, size_t name_len)
{
	mbedtls_conf* mconf = (mbedtls_conf*) arg;

	return mconf->on_sni_callback(ssl, name, name_len);
}

//////////////////////////////////////////////////////////////////////////////

#define CONF_OWN_CERT_NIL	0
#define CONF_OWN_CERT_OK	1
#define CONF_OWN_CERT_ERR	2

#ifdef HAS_MBEDTLS
static acl_pthread_once_t __mbedtls_once = ACL_PTHREAD_ONCE_INIT;
#endif

mbedtls_conf::mbedtls_conf(bool server_side, mbedtls_verify_t verify_mode)
{
#ifdef HAS_MBEDTLS
	acl_pthread_once(&__mbedtls_once, mbedtls_once);
	status_      = __once_inited;

	server_side_ = server_side;
	conf_table_  = NULL;
	conf_        = NULL;
	entropy_     = acl_mycalloc(1, sizeof(mbedtls_entropy_context));
	rnd_         = acl_mycalloc(1, sizeof(mbedtls_ctr_drbg_context));
	ciphers_     = NULL;
	cacert_      = NULL;
	cache_       = NULL;
	verify_mode_ = verify_mode;

	if (status_ == CONF_INIT_OK) {
		__entropy_init((mbedtls_entropy_context*) entropy_);
		__ctr_drbg_init((mbedtls_ctr_drbg_context*) rnd_);

		// Setup cipher_suites
		ciphers_ = __ssl_list_ciphersuites();
		if (ciphers_ == NULL) {
			status_ = CONF_INIT_ERR;
			logger_error("ssl_list_ciphersuites null");
		}
		// 客户端模式下创建缺省的 mbedtls_ssl_config
		else if (!server_side_ && !(conf_ = create_ssl_config())) {
			status_ = CONF_INIT_ERR;
		} else if (!init_rand()) {
			status_ = CONF_INIT_ERR;
		}

	}

	if (status_ == CONF_INIT_ERR) {
		logger_error("Init MbedTLS failed!");
	}
#else
	(void) server_side;
	(void) verify_mode;
	(void) server_side_;
	(void) status_;
	(void) conf_count_;
	(void) conf_table_;
	(void) conf_;
	(void) entropy_;
	(void) rnd_;
	(void) ciphers_;
	(void) cacert_;
	(void) cache_;
	(void) verify_mode_;
#endif
}

mbedtls_conf::~mbedtls_conf(void)
{
#ifdef HAS_MBEDTLS
	free_ca();

	for (size_t i = 0; i != cert_keys_.size(); i++) {
		MBEDTLS_CERT_KEY* ck = cert_keys_[i];

		__x509_crt_free(ck->cert);
		acl_myfree(ck->cert);

		__pk_free((PKEY*) ck->pkey);
		acl_myfree(ck->pkey);

		delete ck;
	}

	for (std::set<mbedtls_ssl_config*>::iterator it = certs_.begin();
		it != certs_.end(); ++it) {

		mbedtls_ssl_config* conf = *it;
		__ssl_config_free(conf);
		acl_myfree(conf);
	}

	delete conf_table_;

	if (status_ != CONF_INIT_NIL) {
		__entropy_free((mbedtls_entropy_context*) entropy_);
	}

	acl_myfree(entropy_);

	// 使用 havege_random 随机数生成器时，在一些虚机上并不能保证随机性,
	// 建议使用 ctr_drbg_random 随机数生成器
#ifdef HAS_HAVEGE
	acl_myfree(rnd_);
#else
	__ctr_drbg_free((mbedtls_ctr_drbg_context*) rnd_);
	acl_myfree(rnd_);
#endif

	if (cache_) {
		__ssl_cache_free(cache_);
		acl_myfree(cache_);
	}
#endif
}

int mbedtls_conf::on_sni_callback(mbedtls_ssl_context* ssl,
	const unsigned char* name, size_t name_len)
{
#ifdef HAS_MBEDTLS
	if (name == NULL || *name == 0 || name_len == 0) {
		return 0;
	}

	string host;
	host.copy(name, name_len);

	MBEDTLS_CERT_KEY* ck = find_ssl_config(host);
	if (ck == NULL) {
		return 0;
	}

	mbedtls_x509_crt*   cert = ck->cert;
	mbedtls_pk_context* pkey = (mbedtls_pk_context*) ck->pkey;
	if (cert == NULL  || pkey == NULL) {
		return 0;
	}

	return __ssl_set_cert(ssl, cert, pkey);
#else
	(void) ssl;
	(void) name;
	(void) name_len;
	return 0;
#endif
}

mbedtls_ssl_config* mbedtls_conf::create_ssl_config(void)
{

#if defined(HAS_MBEDTLS)
	mbedtls_ssl_config* conf = (mbedtls_ssl_config*)
		acl_mycalloc(1, sizeof(mbedtls_ssl_config));

	__ssl_config_init(conf);

# ifdef DEBUG_SSL
	__ssl_conf_dbg(conf, my_debug, stdout);
# endif

	int ret;
	if (server_side_) {
		ret = __ssl_config_defaults(conf,
			MBEDTLS_SSL_IS_SERVER,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
		__ssl_conf_sni(conf, sni_callback, this);
	} else {
		ret = __ssl_config_defaults(conf,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	}

	if (ret != 0) {
		logger_error("ssl_config_defaults error=-0x%04x, side=%s",
			ret, server_side_ ? "server" : "client");
		acl_myfree(conf);
		return NULL;
	}

	// 设置随机数生成器
	__ssl_conf_rng(conf, __ctr_drbg_random,
		(mbedtls_ctr_drbg_context*) rnd_);

	set_authmode(conf, verify_mode_);
	__ssl_conf_endpoint(conf, server_side_ ?
		MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT);
	__ssl_conf_ciphersuites(conf, ciphers_);

	if (conf_ == NULL) {
		conf_ = conf;
	}

	certs_.insert(conf);
	return conf;
#else
	return NULL;
#endif
}

void mbedtls_conf::map_cert(const mbedtls_x509_crt& cert, MBEDTLS_CERT_KEY* ck)
{
	std::vector<string> hosts;
	get_hosts(cert, hosts);

	for (std::vector<string>::iterator cit = hosts.begin();
		cit != hosts.end(); ++cit) {
		bind_host(*cit, ck);
	}
}

void mbedtls_conf::get_hosts(const mbedtls_x509_crt& cert,
	std::vector<string>& hosts)
{
#ifdef HAS_MBEDTLS
	const mbedtls_x509_sequence* san = &cert.subject_alt_names;

#ifndef MBEDTLS_X509_SAN_DNS_NAME
# define MBEDTLS_X509_SAN_DNS_NAME 2
#endif

	acl::string buf;

	while (san) {
		switch ((san->buf.tag & MBEDTLS_X509_SAN_DNS_NAME)) {
		case MBEDTLS_X509_SAN_DNS_NAME:
			if (san->buf.p && san->buf.len > 0) {
				buf.copy(san->buf.p, san->buf.len);
				hosts.push_back(buf);
			}
			//logger("host name=%s", buf.c_str());
			break;
		default:
			break;
		}
		san = san->next;
	}
#else
	(void) cert;
	(void) hosts;
#endif
}

void mbedtls_conf::bind_host(string& host, MBEDTLS_CERT_KEY* ck)
{
	string key;
	if (!create_host_key(host, key)) {
		return;
	}

	if (conf_table_ == NULL) {
		conf_table_ = NEW token_tree;
	}

	if (conf_table_->find(key) == NULL) {
		logger("add host=%s, key=%s", host.c_str(), key.c_str());
		conf_table_->insert(key, ck);
	}
}

bool mbedtls_conf::create_host_key(string& host, string& key, size_t skip /* 0 */)
{
	key.clear();

	std::vector<string>& tokens = host.split2(".");
	if (tokens.empty() || tokens.size() <= skip) {
		return false;
	}

	// Reverse the host name splitted with '.'. for example:
	// www.sina.com --> com.sina.com
	// *.sina.com   --> com.sina.
	// The last char '*' will be changed to '.' above.
	// When skip == 1, then "www.sina.com --> com.sina"

	int size = (int) tokens.size();
	for (int i = size - 1; i >= (int) skip; --i) {
		const string& token = tokens[i];
		if (i != size - 1) {
			key += ".";
		}
		if (token == "*") {
			break;
		}
		key += token;
	}

	return true;
}

MBEDTLS_CERT_KEY* mbedtls_conf::find_ssl_config(const char* host)
{
	string host_buf(host), key;
	if (!create_host_key(host_buf, key)) {
		return NULL;
	}

	//printf(">>>host=%s, key=%s\r\n", host, key.c_str());

	const token_node* node = conf_table_->find(key);
	if (node != NULL) {
		MBEDTLS_CERT_KEY* ck = (MBEDTLS_CERT_KEY*) node->get_ctx();
		return ck;
	}

	// Try the wildcard matching process, and cut off the last item
	// in the host name.
	if (!create_host_key(host_buf, key, 1)) {
		return NULL;
	}

	// The char '.' must be appended in order to wildcard matching,
	// because the wildcard host's lst char must be '.'.
	key += ".";

	node = conf_table_->find(key);
	if (node != NULL) {
		MBEDTLS_CERT_KEY* ck = (MBEDTLS_CERT_KEY*) node->get_ctx();
		return ck;
	}

	//printf("Not found key=%s\r\n", key.c_str());
	return NULL;
}

void mbedtls_conf::free_ca(void)
{
#ifdef HAS_MBEDTLS
	if (cacert_) {
		__x509_crt_free(cacert_);
		acl_myfree(cacert_);
		cacert_ = NULL;
	}
#endif
}

bool mbedtls_conf::load_ca(const char* ca_file, const char* ca_path)
{
#ifdef HAS_MBEDTLS
	free_ca();

	int  ret;

	cacert_ = (mbedtls_x509_crt*) acl_mycalloc(1, sizeof(X509_CRT));
	__x509_crt_init(cacert_);

	if (ca_path && *ca_path) {
		ret = __x509_crt_parse_path(cacert_, ca_path);
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

	ret = __x509_crt_parse_file(cacert_, ca_file);
	if (ret != 0) {
		logger_error("x509_crt_parse_path(%s) error: -0x%04x",
			ca_path, -ret);
		free_ca();
		return false;
	} else {
		// Setup ca cert
		__ssl_conf_ca_chain(conf_, cacert_->next, NULL);
		return true;
	}
#else
	(void) ca_file;
	(void) ca_path;

	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

bool mbedtls_conf::add_cert(const char* crt_file, const char* key_file,
	const char* key_pass)
{
	if (status_ != CONF_INIT_OK) {
		logger_error("MbedTLS not init , status=%d", (int) status_);
		return false;
	}

	if (crt_file == NULL || *crt_file == 0) {
		logger_error("crt_file can't be null nor empty");
		return false;
	}

	if (key_file == NULL || *key_file == 0) {
		logger_error("key_file can't be null nor empty");
		return false;
	}

#ifdef HAS_MBEDTLS
# define FREE_CERT_KEY do { \
	logger_error("add cert (%s:%s) error: -0x%04x", crt_file, key_file, -ret); \
	if (cert) { \
		__x509_crt_free(cert); \
		acl_myfree(cert); \
	} \
	if (pkey) { \
		__pk_free(pkey); \
		acl_myfree(pkey); \
	} \
} while (0)

	mbedtls_ssl_config* conf = create_ssl_config();

	X509_CRT *cert = NULL;
	PKEY *pkey = NULL;

	cert = static_cast<X509_CRT*>(acl_mycalloc(1, sizeof(X509_CRT)));
	__x509_crt_init(cert);
	int ret = __x509_crt_parse_file(cert, crt_file);
	if (ret != 0) {
		FREE_CERT_KEY;
		return false;
	}

	pkey = static_cast<PKEY*>(acl_mycalloc(1, sizeof(PKEY)));
	__pk_init(pkey);
	ret = __pk_parse_keyfile(pkey, key_file, key_pass ? key_pass : "");
	if (ret != 0) {
		FREE_CERT_KEY;
		return false;
	}

	ret = __ssl_conf_own_cert(conf, cert, pkey);
	if (ret != 0) {
		FREE_CERT_KEY;
		return false;
	}

	// Save the cert and pkey just for being freed in ~mbedtls_conf();
	MBEDTLS_CERT_KEY* ck = new MBEDTLS_CERT_KEY;
	ck->cert = cert;
	ck->pkey = pkey;

	cert_keys_.push_back(ck);
	map_cert(*cert, ck);
	return true;
#else
	(void) crt_file;
	(void) key_file;
	(void) key_pass;

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

	crt_file_ = crt_file;
	return true;
}

bool mbedtls_conf::set_key(const char* key_file, const char* key_pass /* NULL */)
{
	if (key_file == NULL || *key_file == 0) {
		logger_error("key_file null");
		return false;
	}

	if (crt_file_.empty()) {
		logger_error("crt_file empty, call add_cert first");
		return false;
	}

	return add_cert(crt_file_, key_file, key_pass);
}

void mbedtls_conf::enable_cache(bool on)
{
#ifdef HAS_MBEDTLS
	if (status_ != CONF_INIT_OK) {
		logger_error("MbedTLS not init , status=%d", (int) status_);
		return;
	}

	if (conf_ == NULL) {
		logger_error("Please call add_cert() first!");
		return;
	}

	if (on) {
		if (cache_ != NULL) {
			return;
		}
		cache_ = (mbedtls_ssl_cache_context*)
			acl_mycalloc(1, sizeof(mbedtls_ssl_cache_context));
		__ssl_cache_init(cache_);
	} else if (cache_ != NULL) {
		__ssl_cache_free(cache_);
		acl_myfree(cache_);
		cache_ = NULL;
	}

	// Setup cache only for server-side
	if (server_side_ && cache_ != NULL) {
		__ssl_conf_session_cache(conf_, cache_,
			__ssl_cache_get, __ssl_cache_set);
	}
#else
	(void) on;
	logger_error("HAS_MBEDTLS not defined!");
#endif
}

bool mbedtls_conf::setup_certs(void* ssl)
{
#ifdef HAS_MBEDTLS
	if (status_ != CONF_INIT_OK) {
		logger_error("MbedTLS not init , status=%d", (int) status_);
		return false;
	}

	int ret = __ssl_setup((mbedtls_ssl_context*) ssl, conf_);
	if (ret != 0) {
		logger_error("ssl_setup error:-0x%04x", -ret);
		return false;
	}

	return true;
#else
	(void) ssl;
	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

sslbase_io* mbedtls_conf::create(bool nblock)
{
	return NEW mbedtls_io(*this, server_side_, nblock);
}

} // namespace acl
