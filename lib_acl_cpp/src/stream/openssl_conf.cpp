#include "acl_stdafx.hpp"

//#define HAS_OPENSSL
//#define HAS_OPENSSL_DLL

#ifdef HAS_OPENSSL_DLL
# ifndef HAS_OPENSSL
#  define HAS_OPENSSL
# endif
#endif

#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stdlib/log.hpp"
# include "acl_cpp/stream/openssl_io.hpp"
# include "acl_cpp/stream/openssl_conf.hpp"
#endif

#ifdef HAS_OPENSSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#endif

#if defined(HAS_OPENSSL)

# if defined(HAS_OPENSSL_DLL)
#  if OPENSSL_VERSION_NUMBER >= 0x10100003L
#   define SSL_INIT_SSL			"OPENSSL_init_ssl"
typedef int (*ssl_init_fn)(uint64_t, const OPENSSL_INIT_SETTINGS*);
static ssl_init_fn __ssl_init;
#  else
#   define SSL_CONFIG			"OPENSSL_config"
typedef void (*ssl_config_fn)(const char*);
static ssl_config_fn __ssl_config;

#   define SSL_LIBRARY_INIT		"SSL_library_init"
typedef int (*ssl_library_init_fn)(void);
static ssl_library_init_fn __ssl_library_init;

#   define SSL_LOAD_ERROR_STRINGS	"SSL_load_error_strings"
typedef void (*ssl_load_error_strings_fn)(void);
static SSL_load_error_strings __ssl_load_error_strings;

#   define SSL_ADD_ALL_ALGORITHMS	"OpenSSL_add_all_algorithms"
typedef void (*ssl_add_all_algorithms_fn)(void);
static ssl_add_all_algorithms_fn __ssl_add_all_algorithms;
#  endif  // OPENSSL_VERSION_NUMBER >= 0x10100003L

#  if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#   ifndef SSL_OP_NO_COMPRESSION
#    define SSL_COMPRESSION_METHODS	"SSL_COMP_get_compression_methods"
typedef STACK_OF(SSL_COMP)* (*ssl_comp_get_methods_fn)(void);
static ssl_comp_get_methods_fn __ssl_comp_get_methods;

#    define SSL_COMP_NUM		"sk_SSL_COMP_num"
typedef int (*ssl_comp_num_fn)(STACK_OF(SSL_COMP)*);
static ssl_comp_num_fn __ssl_comp_num;

#    define SSL_COMP_POP		"sk_SSL_COMP_pop"
typedef int (*ssl_comp_pop_fn)(STACK_OF(SSL_COMP)*);
static ssl_comp_pop __ssl_comp_pop;
#   endif
#  endif // OPENSSL_VERSION_NUMBER >= 0x0090800fL

#  define SSL_CLEAR_ERROR		"ERR_clear_error"
typedef void (*ssl_clear_error_fn)(void);
static ssl_clear_error_fn __ssl_clear_error;

//#  define SSLV23_METHOD		"SSLv23_method"
#  define SSLV23_METHOD			"TLS_method"
typedef const SSL_METHOD* (*sslv23_method_fn)(void);
static sslv23_method_fn __sslv23_method;

#  define SSL_CTX_NEW			"SSL_CTX_new"
typedef SSL_CTX* (*ssl_ctx_new_fn)(const SSL_METHOD*);
static ssl_ctx_new_fn __ssl_ctx_new;

#  define SSL_CTX_FREE			"SSL_CTX_free"
typedef void (*ssl_ctx_free_fn)(SSL_CTX*);
static ssl_ctx_free_fn __ssl_ctx_free;

#  define SSL_CTX_SET_VERIFY_DEPTH		"SSL_CTX_set_verify_depth"
typedef void (*ssl_ctx_set_verify_depth_fn)(SSL_CTX*, int);
static ssl_ctx_set_verify_depth_fn __ssl_ctx_set_verify_depth;

#  define SSL_LOAD_CLIENT_CA		"SSL_load_client_CA_file"
typedef STACK_OF(X509_NAME)* (*ssl_load_client_ca_fn)(const char*);
static ssl_load_client_ca_fn __ssl_load_client_ca;

#  define SSL_CTX_SET_CLIENT_CA	"SSL_CTX_set_client_CA_list"
typedef void (*ssl_ctx_set_client_ca_fn)(SSL_CTX*, STACK_OF(X509_NAME)*);
static ssl_ctx_set_client_ca_fn __ssl_ctx_set_client_ca;

#  define SSL_CTX_USE_CERT_CHAN	"SSL_CTX_use_certificate_chain_file"
typedef int (*ssl_ctx_use_cert_chain_fn)(SSL_CTX*, const char*);
static ssl_ctx_use_cert_chain_fn __ssl_ctx_use_cert_chain;

#  define SSL_CTX_USE_PKEY_FILE	"SSL_CTX_use_PrivateKey_file"
typedef int (*ssl_ctx_use_pkey_fn)(SSL_CTX*, const char*, int);
static ssl_ctx_use_pkey_fn __ssl_ctx_use_pkey;

#  define SSL_CTX_CHECK_PKEY		"SSL_CTX_check_private_key"
typedef int (*ssl_ctx_check_pkey_fn)(const SSL_CTX*);
static ssl_ctx_check_pkey_fn __ssl_ctx_check_pkey;

#  define SSL_CTX_SET_DEF_PASS		"SSL_CTX_set_default_passwd_cb_userdata"
typedef void (*ssl_ctx_set_def_pass_fn)(SSL_CTX*, void*);
static ssl_ctx_set_def_pass_fn __ssl_ctx_set_def_pass;

static acl_pthread_once_t __openssl_once = ACL_PTHREAD_ONCE_INIT;
static acl::string* __crypto_path_buf = NULL;
static acl::string* __ssl_path_buf    = NULL;

#  if defined(_WIN32) || defined(_WIN64)
static const char* __crypto_path = "libcrypto.dll";
static const char* __ssl_path    = "libssl.dll";
#  elif defined(ACL_MACOSX)
static const char* __crypto_path = "libcrypto.dylib";
static const char* __ssl_path    = "libssl.dylib";
#  else
static const char* __crypto_path = "libcrypto.so";
static const char* __ssl_path    = "libssl.so";
#  endif

ACL_DLL_HANDLE __openssl_crypto_dll = NULL;
ACL_DLL_HANDLE __openssl_ssl_dll    = NULL;

extern bool openssl_load_io(void); // defined in openssl_io.cpp

#  ifndef HAVE_NO_ATEXIT
static void openssl_dll_unload(void)
{
	if (__openssl_ssl_dll) {
		acl_dlclose(__openssl_ssl_dll);
		__openssl_ssl_dll = NULL;
	}

	if (__openssl_crypto_dll) {
		acl_dlclose(__openssl_crypto_dll);
		__openssl_crypto_dll = NULL;
	}

	delete __crypto_path_buf;
	__crypto_path_buf = NULL;

	delete __ssl_path_buf;
	__ssl_path_buf = NULL;
}
#  endif

#  define LOAD_SSL(name, type, fn) do {					\
	(fn) = (type) acl_dlsym(__openssl_ssl_dll, (name));		\
	if ((fn) == NULL) {						\
		logger_error("dlsym %s error %s, lib=%s",		\
			name, acl_dlerror(), __ssl_path);		\
		return false;						\
	}								\
} while (0)

static bool load_from_ssl(void)
{
#  if OPENSSL_VERSION_NUMBER >= 0x10100003L
	LOAD_SSL(SSL_INIT_SSL, ssl_init_fn, __ssl_init);
#else
	LOAD_SSL(SSL_CONFIG, ssl_config_fn, __ssl_config);
	LOAD_SSL(SSL_LIBRARY_INIT, ssl_library_init_fn, __ssl_library_init);
	LOAD_SSL(SSL_LOAD_ERROR_STRINGS, SSL_load_error_strings, __ssl_load_error_strings);
	LOAD_SSL(SSL_ADD_ALL_ALGORITHMS, ssl_add_all_algorithms_fn, __ssl_add_all_algorithms;);
#endif

#  if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#   ifndef SSL_OP_NO_COMPRESSION
	LOAD_SSL(SSL_COMPRESSION_METHODS, ssl_comp_get_methods_fn, __ssl_comp_get_methods);
	LOAD_SSL(SSL_COMP_NUM, ssl_comp_num_fn, __ssl_comp_num);
	LOAD_SSL(SSL_COMP_POP, ssl_comp_pop, __ssl_comp_pop);
#   endif
#  endif

	LOAD_SSL(SSL_CLEAR_ERROR, ssl_clear_error_fn, __ssl_clear_error);
	LOAD_SSL(SSLV23_METHOD, sslv23_method_fn, __sslv23_method);
	LOAD_SSL(SSL_CTX_NEW, ssl_ctx_new_fn, __ssl_ctx_new);
	LOAD_SSL(SSL_CTX_FREE, ssl_ctx_free_fn, __ssl_ctx_free);
	LOAD_SSL(SSL_CTX_SET_VERIFY_DEPTH, ssl_ctx_set_verify_depth_fn, __ssl_ctx_set_verify_depth);
	LOAD_SSL(SSL_LOAD_CLIENT_CA, ssl_load_client_ca_fn, __ssl_load_client_ca);
	LOAD_SSL(SSL_CTX_SET_CLIENT_CA, ssl_ctx_set_client_ca_fn, __ssl_ctx_set_client_ca);
	LOAD_SSL(SSL_CTX_USE_CERT_CHAN, ssl_ctx_use_cert_chain_fn, __ssl_ctx_use_cert_chain);
	LOAD_SSL(SSL_CTX_USE_PKEY_FILE, ssl_ctx_use_pkey_fn, __ssl_ctx_use_pkey);
	LOAD_SSL(SSL_CTX_CHECK_PKEY, ssl_ctx_check_pkey_fn, __ssl_ctx_check_pkey);
	LOAD_SSL(SSL_CTX_SET_DEF_PASS, ssl_ctx_set_def_pass_fn, __ssl_ctx_set_def_pass);
	return true;
}

static bool load_all_dlls(void)
{
	if (__crypto_path_buf && !__crypto_path_buf->empty()) {
		__crypto_path = __crypto_path_buf->c_str();
	}

	if (__ssl_path_buf && !__ssl_path_buf->empty()) {
		__ssl_path = __ssl_path_buf->c_str();
	}

	__openssl_crypto_dll = acl_dlopen(__crypto_path);
	if (__openssl_crypto_dll == NULL) {
		logger_error("load %s error %s", __crypto_path, acl_dlerror());
		return false;
	}

	__openssl_ssl_dll = acl_dlopen(__ssl_path);
	if (__openssl_ssl_dll == NULL) {
		logger_error("load %s error %s", __ssl_path, acl_dlerror());
		return false;
	}

	return true;
}

static void openssl_dll_load(void)
{
	if (__openssl_ssl_dll) {
		logger("openssl(%s) has been loaded!", __ssl_path);
		return;
	}

	if (!load_all_dlls()) {
		return;
	}

	if (!load_from_ssl()) {
		acl_dlclose(__openssl_ssl_dll);
		__openssl_ssl_dll = NULL;
		return;
	}

	if (!openssl_load_io()) {
		logger_error("openssl_dll_load_io %s error", __ssl_path);
		acl_dlclose(__openssl_ssl_dll);
		__openssl_ssl_dll = NULL;
		return;
	}

	logger("%s, %s loaded!", __crypto_path, __ssl_path);

#ifndef HAVE_NO_ATEXIT
	atexit(openssl_dll_unload);
#endif
}

# else  // !HAS_OPENSSL_DLL && HAS_OPENSSL

#  if OPENSSL_VERSION_NUMBER >= 0x10100003L
#   define __ssl_init			OPENSSL_init_ssl
#  else
#   define __ssl_config			OPENSSL_config
#   define __ssl_library_init		SSL_library_init
#   define __ssl_load_error_strings	SSL_load_error_strings
#   define __ssl_add_all_algorithms	OpenSSL_add_all_algorithms
#  endif  // OPENSSL_VERSION_NUMBER >= 0x10100003L

#  if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#   ifndef SSL_OP_NO_COMPRESSION
#    define __ssl_comp_get_methods	SSL_COMP_get_compression_methods
#    define __ssl_comp_num		sk_SSL_COMP_num
#    define __ssl_comp_pop		sk_SSL_COMP_pop
#   endif
#  endif // OPENSSL_VERSION_NUMBER >= 0x0090800fL

#  define __ssl_clear_error 		ERR_clear_error
#  define __sslv23_method		SSLv23_method
#  define __ssl_ctx_new			SSL_CTX_new
#  define __ssl_ctx_free		SSL_CTX_free
#  define __ssl_ctx_set_verify_depth	SSL_CTX_set_verify_depth
#  define __ssl_load_client_ca		SSL_load_client_CA_file
#  define __ssl_ctx_set_client_ca	SSL_CTX_set_client_CA_list
#  define __ssl_ctx_use_cert_chain	SSL_CTX_use_certificate_chain_file
#  define __ssl_ctx_use_pkey		SSL_CTX_use_PrivateKey_file
#  define __ssl_ctx_check_pkey		SSL_CTX_check_private_key
#  define __ssl_ctx_set_def_pass	SSL_CTX_set_default_passwd_cb_userdata
# endif // !HAS_OPENSSL_DLL

#endif  // HAS_OPENSSL

namespace acl {

#define CONF_INIT_NIL	0
#define CONF_INIT_OK	1
#define CONF_INIT_ERR	2

void openssl_conf::set_libpath(const char* libcrypto, const char* libssl)
{
#ifdef HAS_OPENSSL_DLL
	if (__crypto_path_buf == NULL) {
		__crypto_path_buf = NEW string;
	}
	*__crypto_path_buf = libcrypto;

	if (__ssl_path_buf == NULL) {
		__ssl_path_buf = NEW string;
	}
	*__ssl_path_buf = libssl;
#else
	(void) libcrypto;
	(void) libssl;
#endif
}

bool openssl_conf::load(void)
{
#ifdef HAS_OPENSSL_DLL
	acl_pthread_once(&__openssl_once, openssl_dll_load);
	if (__openssl_ssl_dll == NULL) {
		logger_error("load openssl error");
		return false;
	}
	return true;
#else
	logger_warn("link openssl library in static way!");
	return true;
#endif
}

bool openssl_conf::init_once(void)
{
#ifdef HAS_OPENSSL_DLL
	if (!load()) {
		return false;
	}
#endif

#ifdef HAS_OPENSSL
	thread_mutex_guard guard(lock_);
	if (init_status_ == CONF_INIT_OK) {
		return true;
	} else if (init_status_ == CONF_INIT_ERR) {
		return false;
	}
	assert(init_status_ == CONF_INIT_NIL);

	ssl_ctx_ = (void*) __ssl_ctx_new(__sslv23_method());

# if OPENSSL_VERSION_NUMBER >= 0x10100003L
	if (__ssl_init(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
		logger_error("OPENSSL_init_ssl error");
		init_status_ = CONF_INIT_ERR;
		return false;
	}

	// OPENSSL_init_ssl() may leave errors in the error queue
	// while returning success -- nginx
	__ssl_clear_error();
# else
	__ssl_config(NULL);
	__ssl_library_init();
	__ssl_load_error_strings();
	__ssl_add_all_algorithms();
# endif

# if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#  ifndef SSL_OP_NO_COMPRESSION
	// Disable gzip compression in OpenSSL prior to 1.0.0 version,
	// this saves about 522K per connection. -- nginx
	STACK_OF(SSL_COMP)* ssl_comp_methods = __ssl_comp_get_methods();
	int n = __ssl_comp_num(ssl_comp_methods);
	while (n--) {
		(void) __ssl_comp_pop(ssl_comp_methods);
	}
#  endif
# endif
	init_status_ = CONF_INIT_OK;
	return true;
#else
	logger_error("HAS_OPENSSL not defined!");
	return false;
#endif // HAS_OPENSSL
}

openssl_conf::openssl_conf(bool server_side /* false */)
: server_side_(server_side)
, ssl_ctx_(NULL)
, init_status_(CONF_INIT_NIL)
{
}

openssl_conf::~openssl_conf(void)
{
#ifdef HAS_OPENSSL
	if (ssl_ctx_) {
		__ssl_ctx_free((SSL_CTX *) ssl_ctx_);
	}
#endif
}

bool openssl_conf::load_ca(const char* ca_file, const char* /* ca_path */)
{
	if (ca_file == NULL) {
		logger_error("ca_file NULL");
		return false;
	}

	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}

#ifdef HAS_OPENSSL
	__ssl_ctx_set_verify_depth((SSL_CTX*) ssl_ctx_, 5);

	STACK_OF(X509_NAME)* list = __ssl_load_client_ca(ca_file);
	if (list == NULL) {
		logger_error("load CA file(%s) error", ca_file);
		return false;
	}

	// Before 0.9.7h and 0.9.8 SSL_load_client_CA_file()
	// always leaved an error in the error queue. -- nginx
	__ssl_clear_error();

	__ssl_ctx_set_client_ca((SSL_CTX*) ssl_ctx_, list);
	return true;
#else
	logger_error("HAS_OPENSSL not defined!");
	return false;
#endif
}

bool openssl_conf::add_cert(const char* crt_file, const char* key_file,
	const char* key_pass /* NULL */)
{
	if (crt_file == NULL || *crt_file == 0) {
		logger_error("crt_file empty");
		return false;
	}

	if (key_file == NULL || *key_file == 0) {
		logger_error("key_file empty");
		return false;
	}

	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}

#ifdef HAS_OPENSSL
	SSL_CTX* ssl_ctx = (SSL_CTX*) ssl_ctx_;

	if (!__ssl_ctx_use_cert_chain(ssl_ctx, crt_file)) {
		logger_error("use crt chain file(%s) error", crt_file);
		return false;
	}

	if (!__ssl_ctx_use_pkey(ssl_ctx, key_file, SSL_FILETYPE_PEM)) {
		logger_error("load private key(%s) error", key_file);
		return false;
	}

	if (!__ssl_ctx_check_pkey(ssl_ctx)) {
		logger_error("check private key(%s) error", key_file);
		return false;
	}

	if (key_pass && *key_pass) {
		__ssl_ctx_set_def_pass(ssl_ctx, (void*) key_pass);
	}

	return true;
#else
	(void) key_pass;
	logger_error("HAS_OPENSSL not defined!");
	return false;
#endif
}

bool openssl_conf::add_cert(const char* crt_file)
{
	if (crt_file == NULL || *crt_file == 0) {
		logger_error("crt_file null");
		return false;
	}

	crt_file_ = crt_file;
	return true;
}

bool openssl_conf::set_key(const char* key_file, const char* key_pass /* NULL */)
{
	if (crt_file_.empty()) {
		logger_error("call add_cert first");
		return false;
	}

	return add_cert(crt_file_, key_file, key_pass);
}

void openssl_conf::enable_cache(bool /* on */)
{
	if (!init_once()) {
		logger_error("init_once error");
	}
}

bool openssl_conf::setup_certs(void* ssl)
{
	(void) ssl;

	if (!init_once()) {
		logger_error("init_once error");
		return false;
	}
	return true;
}

sslbase_io* openssl_conf::create(bool nblock)
{
	return NEW openssl_io(*this, server_side_, nblock);
}

} // namespace acl
