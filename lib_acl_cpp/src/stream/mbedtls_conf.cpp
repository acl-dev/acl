#include "acl_stdafx.hpp"

#ifdef HAS_MBEDTLS_DLL
# ifndef HAS_MBEDTLS
#  define HAS_MBEDTLS
# endif
#endif

//#define DEBUG_SSL

#ifdef HAS_MBEDTLS
# include "mbedtls-2.7.12/threading_alt.h"
# include "mbedtls-2.7.12/ssl.h"
# include "mbedtls-2.7.12/ctr_drbg.h"
# include "mbedtls-2.7.12/entropy.h"
# include "mbedtls-2.7.12/certs.h"
# include "mbedtls-2.7.12/x509_crt.h"
# include "mbedtls-2.7.12/x509.h"
# include "mbedtls-2.7.12/ssl_cache.h"
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
#  ifdef DEBUG_SSL
#   define SSL_CONF_DBG_NAME		"mbedtls_ssl_conf_dbg"
#  endif

#  define SSL_CACHE_INIT_NAME		"mbedtls_ssl_cache_init"
#  define SSL_CACHE_FREE_NAME		"mbedtls_ssl_cache_free"
#  define SSL_CACHE_SET_NAME		"mbedtls_ssl_cache_set"
#  define SSL_CACHE_GET_NAME		"mbedtls_ssl_cache_get"

#  define SSL_SETUP_NAME		"mbedtls_ssl_setup"

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
# ifdef DEBUG_SSL
static ssl_conf_dbg_fn			__ssl_conf_dbg;
# endif

static ssl_cache_init_fn		__ssl_cache_init;
static ssl_cache_free_fn		__ssl_cache_free;
static ssl_cache_set_fn			__ssl_cache_set;
static ssl_cache_get_fn			__ssl_cache_get;

static ssl_setup_fn			__ssl_setup;

static acl_pthread_once_t __mbedtls_once = ACL_PTHREAD_ONCE_INIT;
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
		logger("%s unload ok", __crypto_path);
	}
	if (__x509_dll && __x509_dll != __crypto_dll) {
		acl_dlclose(__x509_dll);
		logger("%s unload ok", __x509_path);
	}
	if (__tls_dll && __tls_dll != __crypto_dll) {
		acl_dlclose(__tls_dll);
		logger("%s unload ok", __tls_path);
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
		logger_error("dlsym %s error %s", name, acl_dlerror());	\
		return false;						\
	}								\
} while (0)

#define LOAD_X509(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__x509_dll, (name));			\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s", name, acl_dlerror());	\
		return false;						\
	}								\
} while (0)

#define LOAD_SSL(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__tls_dll, (name));			\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s", name, acl_dlerror());	\
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
# ifdef DEBUG_SSL
	LOAD_SSL(SSL_CONF_DBG_NAME, ssl_conf_dbg_fn, __ssl_conf_dbg);
# endif

	LOAD_SSL(SSL_CACHE_INIT_NAME, ssl_cache_init_fn, __ssl_cache_init);
	LOAD_SSL(SSL_CACHE_FREE_NAME, ssl_cache_free_fn, __ssl_cache_free);
	LOAD_SSL(SSL_CACHE_SET_NAME, ssl_cache_set_fn, __ssl_cache_set);
	LOAD_SSL(SSL_CACHE_GET_NAME, ssl_cache_get_fn, __ssl_cache_get);

	LOAD_SSL(SSL_SETUP_NAME, ssl_setup_fn, __ssl_setup);
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
#  ifdef DEBUG_SSL
#   define __ssl_conf_dbg		::mbedtls_ssl_conf_dbg
#  endif
#  define __ssl_cache_init		::mbedtls_ssl_cache_init
#  define __ssl_cache_free		::mbedtls_ssl_cache_free
#  define __ssl_cache_set		::mbedtls_ssl_cache_set
#  define __ssl_cache_get		::mbedtls_ssl_cache_get
#  define __ssl_setup			::mbedtls_ssl_setup

# endif // HAS_MBEDTLS_DLL
#endif  // HAS_MBEDTLS

namespace acl
{

void mbedtls_conf::set_libpath(const char* libmbedcrypto,
	const char* libmbedx509, const char* libmbedtls)
{
#ifdef HAS_MBEDTLS_DLL
	if (__crypto_path_buf == NULL) {
		__crypto_path_buf = NEW string;
	}
	* __crypto_path_buf = libmbedcrypto;

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
	acl_pthread_once(&__mbedtls_once, mbedtls_dll_load);
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

	// 设置随机数生成器
	__ssl_conf_rng((mbedtls_ssl_config*) conf_, __ctr_drbg_random,
		(mbedtls_ctr_drbg_context*) rnd_);
	return true;
#else
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

#define CONF_INIT_NIL	0
#define CONF_INIT_OK	1
#define CONF_INIT_ERR	2

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

bool mbedtls_conf::init_once(void)
{
#ifdef HAS_MBEDTLS_DLL
	if (!load()) {
		return false;
	}
#endif

	thread_mutex_guard guard(lock_);
	if (init_status_ == CONF_INIT_OK) {
		return true;
	} else if (init_status_ == CONF_INIT_ERR) {
		return false;
	}
	assert(init_status_ == CONF_INIT_NIL);

#if defined(HAS_MBEDTLS)
# if defined(MBEDTLS_THREADING_ALT)
	__threading_set_alt(mutex_init, mutex_free, mutex_lock, mutex_unlock);
# endif
	__ssl_config_init((mbedtls_ssl_config*) conf_);
	__entropy_init((mbedtls_entropy_context*) entropy_);
	__ctr_drbg_init((mbedtls_ctr_drbg_context*) rnd_);
# ifdef DEBUG_SSL
	__ssl_conf_dbg((mbedtls_ssl_config*) conf_, my_debug, stdout);
# endif

	int ret;
	if (server_side_) {
		ret = __ssl_config_defaults((mbedtls_ssl_config*) conf_,
			MBEDTLS_SSL_IS_SERVER,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	} else {
		ret = __ssl_config_defaults((mbedtls_ssl_config*) conf_,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	}
	if (ret != 0) {
		init_status_ = CONF_INIT_ERR;
		logger_error("ssl_config_defaults error=-0x%04x, side=%s",
			ret, server_side_ ? "server" : "client");
		return false;
	}

	if (!init_rand()) {
		init_status_ = CONF_INIT_ERR;
		return false;
	}

	set_authmode((mbedtls_ssl_config*) conf_, verify_mode_);
	__ssl_conf_endpoint((mbedtls_ssl_config*) conf_, server_side_ ?
		MBEDTLS_SSL_IS_SERVER : MBEDTLS_SSL_IS_CLIENT);

	// Setup cipher_suites
	const int* cipher_suites = __ssl_list_ciphersuites();
	if (cipher_suites == NULL) {
		init_status_ = CONF_INIT_ERR;
		logger_error("ssl_list_ciphersuites null");
		return false;
	}

	__ssl_conf_ciphersuites((mbedtls_ssl_config*) conf_, cipher_suites);
#endif
	init_status_ = CONF_INIT_OK;
	return true;
}

#define CONF_OWN_CERT_NIL	0
#define CONF_OWN_CERT_OK	1
#define CONF_OWN_CERT_ERR	2

mbedtls_conf::mbedtls_conf(bool server_side, mbedtls_verify_t verify_mode)
{
#ifdef HAS_MBEDTLS
	server_side_ = server_side;
	init_status_ = CONF_INIT_NIL;
	cert_status_ = CONF_OWN_CERT_NIL;
	conf_        = acl_mycalloc(1, sizeof(mbedtls_ssl_config));
	entropy_     = acl_mycalloc(1, sizeof(mbedtls_entropy_context));
	rnd_         = acl_mycalloc(1, sizeof(mbedtls_ctr_drbg_context));
	cacert_      = NULL;
	cert_chain_  = NULL;
	
	cache_       = NULL;
	pkey_        = NULL;
	verify_mode_ = verify_mode;
#else
	(void) server_side_;
	(void) init_status_;
	(void) cert_status_;
	(void) conf_;
	(void) entropy_;
	(void) rnd_;
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
		__x509_crt_free((X509_CRT*) cert_chain_);
		acl_myfree(cert_chain_);
	}

	if (pkey_) {
		__pk_free((PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	if (init_status_ != CONF_INIT_NIL) {
		__entropy_free((mbedtls_entropy_context*) entropy_);
	}

	for (size_t i = 0; i != cert_keys_.size(); i++) {
		void* cert = cert_keys_[i].first;
		__x509_crt_free((X509_CRT*) cert);
		acl_myfree(cert);

		void* pkey = cert_keys_[i].second;
		__pk_free((PKEY*) pkey);
		acl_myfree(pkey);
	}
	__ssl_config_free((mbedtls_ssl_config*) conf_);
	acl_myfree(conf_);
	acl_myfree(entropy_);

	// 使用 havege_random 随机数生成器时，在一些虚机上并不能保证随机性,
	// 建议使用 ctr_drbg_random 随机数生成器
#ifdef HAS_HAVEGE
	acl_myfree(rnd_);
#else
	__ctr_drbg_free((mbedtls_ctr_drbg_context*) rnd_);
#endif

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
		__x509_crt_free((X509_CRT*) cacert_);
		acl_myfree(cacert_);
		cacert_ = NULL;
	}
#endif
}

bool mbedtls_conf::load_ca(const char* ca_file, const char* ca_path)
{
#ifdef HAS_MBEDTLS
	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}

	free_ca();

	int  ret;

	cacert_ = acl_mycalloc(1, sizeof(X509_CRT));
	__x509_crt_init((X509_CRT*) cacert_);

	if (ca_path && *ca_path) {
		ret = __x509_crt_parse_path((X509_CRT*) cacert_, ca_path);
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

	ret = __x509_crt_parse_file((X509_CRT*) cacert_, ca_file);
	if (ret != 0) {
		logger_error("x509_crt_parse_path(%s) error: -0x%04x",
			ca_path, -ret);
		free_ca();
		return false;
	} else {
		// Setup ca cert
		__ssl_conf_ca_chain((mbedtls_ssl_config*) conf_,
			((X509_CRT*) cacert_)->next, NULL);
		return true;
	}
#else
	(void) ca_file;
	(void) ca_path;

	logger_error("HAS_MBEDTLS not defined!");
	return false;
#endif
}

bool mbedtls_conf::append_key_cert(const char* crt_file, const char* key_file,
	const char* key_pass)
{
	if (crt_file == NULL || crt_file[0] == '\0' ||
		key_file == NULL || key_file[0] == '\0') {
		logger_error("crt_file or key_file null");
		return false;
	}

#ifdef HAS_MBEDTLS
	int ret = 0;
	X509_CRT *cert = NULL;
	PKEY *pkey = NULL;

	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}

	cert = static_cast<X509_CRT*>(acl_mycalloc(1, sizeof(X509_CRT)));
	__x509_crt_init(cert);
	ret = __x509_crt_parse_file(cert, crt_file);
	if (ret != 0) {
		goto ERR;
	}

	pkey = static_cast<PKEY*>(acl_mycalloc(1, sizeof(PKEY)));
	__pk_init(pkey);
	ret = __pk_parse_keyfile(pkey, key_file, key_pass ? key_pass : "");
	if (ret != 0) {
		goto ERR;
	}

	ret = __ssl_conf_own_cert((mbedtls_ssl_config*)conf_, cert, pkey);
	if (ret != 0) {
		goto ERR;
	}

	cert_keys_.push_back(std::make_pair(cert, pkey));
	cert_status_ = CONF_OWN_CERT_OK;
	return true;
ERR:
	logger_error("append_key_cert(%s:%s) error: -0x%04x",
		crt_file, key_file, -ret);
	if (cert) {
		__x509_crt_free(cert);
		acl_myfree(cert);
	}

	if (pkey) {
		__pk_free(pkey);
		acl_myfree(pkey);
	}
	return false;
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

#ifdef HAS_MBEDTLS
	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}

	if (cert_chain_ == NULL) {
		cert_chain_ = acl_mycalloc(1, sizeof(X509_CRT));
		__x509_crt_init((X509_CRT*) cert_chain_);
	}

	int ret = __x509_crt_parse_file((X509_CRT*) cert_chain_, crt_file);
	if (ret != 0) {
		logger_error("x509_crt_parse_file(%s) error: -0x%04x",
			crt_file, -ret);

		__x509_crt_free((X509_CRT*) cert_chain_);
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
	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}

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
			key_file, -ret);

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
	if (!init_once()) {
		logger_error("init_once error");
		return;
	}

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
	// Setup cache only for server-side
	if (server_side_ && cache_ != NULL) {
		__ssl_conf_session_cache((mbedtls_ssl_config*) conf_, cache_,
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
	if (!init_once()) {
		return false;
	}

	int ret = __ssl_setup((mbedtls_ssl_context*) ssl,
			(mbedtls_ssl_config*) conf_);
	if (ret != 0) {
		logger_error("ssl_setup error:-0x%04x", -ret);
		return false;
	}

	if (cert_chain_ == NULL || pkey_ == NULL) {
		return true;
	}

	thread_mutex_guard guard(lock_);
	if (cert_status_ == CONF_OWN_CERT_OK) {
		return true;
	} else if (cert_status_ == CONF_OWN_CERT_ERR) {
		return false;
	}

	// Setup own's cert chain and private key
	ret = __ssl_conf_own_cert((mbedtls_ssl_config*) conf_,
			(X509_CRT*) cert_chain_, (PKEY*) pkey_);
	if (ret != 0) {
		cert_status_ = CONF_OWN_CERT_ERR;
		logger_error("ssl_conf_own_cert error: -0x%04x", -ret);
		return false;
	}
	cert_status_ = CONF_OWN_CERT_OK;
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
