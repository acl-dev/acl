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
# include "acl_cpp/stdlib/token_tree.hpp"
# include "acl_cpp/stream/openssl_io.hpp"
# include "acl_cpp/stream/openssl_conf.hpp"
#endif

#ifdef HAS_OPENSSL
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "openssl/x509.h"
#include "openssl/x509v3.h"
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

#  define SSLV23_METHOD			"TLS_method"
typedef const SSL_METHOD* (*sslv23_method_fn)(void);
static sslv23_method_fn __sslv23_method;

#  define SSL_CTX_NEW			"SSL_CTX_new"
typedef SSL_CTX* (*ssl_ctx_new_fn)(const SSL_METHOD*);
static ssl_ctx_new_fn __ssl_ctx_new;

#  define SSL_CTX_FREE			"SSL_CTX_free"
typedef void (*ssl_ctx_free_fn)(SSL_CTX*);
static ssl_ctx_free_fn __ssl_ctx_free;

#  define SSL_CTX_SET_VERIFY_DEPTH	"SSL_CTX_set_verify_depth"
typedef void (*ssl_ctx_set_verify_depth_fn)(SSL_CTX*, int);
static ssl_ctx_set_verify_depth_fn __ssl_ctx_set_verify_depth;

#  define SSL_LOAD_CLIENT_CA		"SSL_load_client_CA_file"
typedef STACK_OF(X509_NAME)* (*ssl_load_client_ca_fn)(const char*);
static ssl_load_client_ca_fn __ssl_load_client_ca;

#  define SSL_CTX_SET_CLIENT_CA		"SSL_CTX_set_client_CA_list"
typedef void (*ssl_ctx_set_client_ca_fn)(SSL_CTX*, STACK_OF(X509_NAME)*);
static ssl_ctx_set_client_ca_fn __ssl_ctx_set_client_ca;

#  define SSL_CTX_USE_CERT_CHAN		"SSL_CTX_use_certificate_chain_file"
typedef int (*ssl_ctx_use_cert_chain_fn)(SSL_CTX*, const char*);
static ssl_ctx_use_cert_chain_fn __ssl_ctx_use_cert_chain;

#  define SSL_CTX_USE_CERT		"SSL_CTX_use_certificate_file"
typedef int (*ssl_ctx_use_cert_fn)(SSL_CTX*, const char*, int type);
static ssl_ctx_use_cert_fn __ssl_ctx_use_cert;

#  define SSL_CTX_USE_PKEY_FILE		"SSL_CTX_use_PrivateKey_file"
typedef int (*ssl_ctx_use_pkey_fn)(SSL_CTX*, const char*, int);
static ssl_ctx_use_pkey_fn __ssl_ctx_use_pkey;

#  define SSL_CTX_CHECK_PKEY		"SSL_CTX_check_private_key"
typedef int (*ssl_ctx_check_pkey_fn)(const SSL_CTX*);
static ssl_ctx_check_pkey_fn __ssl_ctx_check_pkey;

#  define SSL_CTX_SET_DEF_PASS		"SSL_CTX_set_default_passwd_cb_userdata"
typedef void (*ssl_ctx_set_def_pass_fn)(SSL_CTX*, void*);
static ssl_ctx_set_def_pass_fn __ssl_ctx_set_def_pass;

#  define SSL_CTX_SET_TIMEOUT		"SSL_CTX_set_timeout"
typedef long (*ssl_ctx_set_timeout_fn)(SSL_CTX*, long);
static ssl_ctx_set_timeout_fn __ssl_ctx_set_timeout;

#  define SSL_CTX_CALLBACK_CTRL		"SSL_CTX_callback_ctrl"
typedef long (*ssl_ctx_callback_ctrl_fn)(SSL_CTX*, int, void (*)(void));
static ssl_ctx_callback_ctrl_fn __ssl_ctx_callback_ctrl;

#  define SSL_CTX_CTRL			"SSL_CTX_ctrl"
typedef long (*ssl_ctx_ctrl_fn)(SSL_CTX*, int, long, void*);
static ssl_ctx_ctrl_fn __ssl_ctx_ctrl;

#  define ASN1_STRING_PRINT_EX		"ASN1_STRING_print_ex"
typedef int (*asn1_string_print_ex_fn)(BIO*, const ASN1_STRING*, unsigned long);
static asn1_string_print_ex_fn __asn1_string_print_ex;

#  define BIO_NEW			"BIO_new"
typedef BIO *(*bio_new_fn)(const BIO_METHOD*);
static bio_new_fn __bio_new;

#  define BIO_S_MEM			"BIO_s_mem"
typedef const BIO_METHOD *(*bio_s_mem_fn)(void);
static bio_s_mem_fn __bio_s_mem;

#  define BIO_CTRL			"BIO_ctrl"
typedef long (*bio_ctrl_fn)(BIO*, int, long, void*);
static bio_ctrl_fn __bio_ctrl;

#  define BIO_READ			"BIO_read"
typedef int (*bio_read_fn)(BIO*, void*, int);
static bio_read_fn __bio_read;

#  define BIO_FREE			"BIO_free"
typedef int (*bio_free_fn)(BIO*);;
static bio_free_fn __bio_free;

#  define SSL_CTX_GET0_CERTIFICATE	"SSL_CTX_get0_certificate"
typedef X509 *(*ssl_ctx_get0_certificate_fn)(const SSL_CTX*);
static ssl_ctx_get0_certificate_fn __ssl_ctx_get0_certificate;

#  define X509_GET_EXT_D2I		"X509_get_ext_d2i"
typedef void *(*x509_get_ext_d2i_fn)(const X509*, int, int*, int*);
static x509_get_ext_d2i_fn __x509_get_ext_d2i;

#  define SSL_GET_SERVERNAME		"SSL_get_servername"
typedef const char *(*ssl_get_servername_fn)(const SSL*, const int);
static ssl_get_servername_fn __ssl_get_servername;

#  define SSL_GET_SSL_CTX		"SSL_get_SSL_CTX"
typedef SSL_CTX *(*ssl_get_ssl_ctx_fn)(const SSL*);
static ssl_get_ssl_ctx_fn __ssl_get_ssl_ctx;

#  define SSL_SET_SSL_CTX		"SSL_set_SSL_CTX"
typedef SSL_CTX *(*ssl_set_ssl_ctx_fn)(SSL*, SSL_CTX*);
static ssl_set_ssl_ctx_fn __ssl_set_ssl_ctx;

#  define SSL_CTX_GET_OPTION		"SSL_CTX_get_options"
typedef unsigned long (*ssl_ctx_get_options_fn)(const SSL_CTX*);
static ssl_ctx_get_options_fn __ssl_ctx_get_options;

#  define SSL_SET_OPTIONS		"SSL_set_options"
typedef unsigned long (*ssl_set_options_fn)(SSL*, unsigned long);
static ssl_set_options_fn __ssl_set_options;

#define OPENSSL_SK_NUM			"OPENSSL_sk_num"
typedef int (*openssl_sk_num_fn)(const OPENSSL_STACK*);
static openssl_sk_num_fn __openssl_sk_num;

#define OPENSSL_SK_VALUE		"OPENSSL_sk_value"
typedef void *(*openssl_sk_value_fn)(const OPENSSL_STACK*, int);
static openssl_sk_value_fn __openssl_sk_value;

//////////////////////////////////////////////////////////////////////////////

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

ACL_DLL_HANDLE __openssl_crypto_dll   = NULL;
ACL_DLL_HANDLE __openssl_ssl_dll      = NULL;
static acl_pthread_once_t __load_once = ACL_PTHREAD_ONCE_INIT;

//////////////////////////////////////////////////////////////////////////////

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

#  define LOAD_SSL(name, type, fn) do {                     \
    (fn) = (type) acl_dlsym(__openssl_ssl_dll, (name));     \
    if ((fn) == NULL) {                                     \
        logger_error("dlsym %s error %s, lib=%s",           \
            name, acl_dlerror(), __ssl_path);               \
        return false;                                       \
    }                                                       \
} while (0)

#  define LOAD_CRYPTO(name, type, fn) do {                  \
    (fn) = (type) acl_dlsym(__openssl_crypto_dll, (name));  \
    if ((fn) == NULL) {                                     \
        logger_error("dlsym %s error %s, lib=%s",           \
            name, acl_dlerror(), __ssl_path);               \
        return false;                                       \
    }                                                       \
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

#  if defined(_WIN32) || defined(_WIN64)
	LOAD_CRYPTO(SSL_CLEAR_ERROR, ssl_clear_error_fn, __ssl_clear_error);
#else
	LOAD_SSL(SSL_CLEAR_ERROR, ssl_clear_error_fn, __ssl_clear_error);
#endif
	LOAD_SSL(SSLV23_METHOD, sslv23_method_fn, __sslv23_method);
	LOAD_SSL(SSL_CTX_NEW, ssl_ctx_new_fn, __ssl_ctx_new);
	LOAD_SSL(SSL_CTX_FREE, ssl_ctx_free_fn, __ssl_ctx_free);
	LOAD_SSL(SSL_CTX_SET_VERIFY_DEPTH, ssl_ctx_set_verify_depth_fn, __ssl_ctx_set_verify_depth);
	LOAD_SSL(SSL_LOAD_CLIENT_CA, ssl_load_client_ca_fn, __ssl_load_client_ca);
	LOAD_SSL(SSL_CTX_SET_CLIENT_CA, ssl_ctx_set_client_ca_fn, __ssl_ctx_set_client_ca);
	LOAD_SSL(SSL_CTX_USE_CERT_CHAN, ssl_ctx_use_cert_chain_fn, __ssl_ctx_use_cert_chain);
	LOAD_SSL(SSL_CTX_USE_CERT, ssl_ctx_use_cert_fn, __ssl_ctx_use_cert);
	LOAD_SSL(SSL_CTX_USE_PKEY_FILE, ssl_ctx_use_pkey_fn, __ssl_ctx_use_pkey);
	LOAD_SSL(SSL_CTX_CHECK_PKEY, ssl_ctx_check_pkey_fn, __ssl_ctx_check_pkey);
	LOAD_SSL(SSL_CTX_SET_DEF_PASS, ssl_ctx_set_def_pass_fn, __ssl_ctx_set_def_pass);
	LOAD_SSL(SSL_CTX_SET_TIMEOUT, ssl_ctx_set_timeout_fn, __ssl_ctx_set_timeout);
	LOAD_SSL(SSL_CTX_CALLBACK_CTRL, ssl_ctx_callback_ctrl_fn, __ssl_ctx_callback_ctrl);
	LOAD_SSL(SSL_CTX_CTRL, ssl_ctx_ctrl_fn, __ssl_ctx_ctrl);
	LOAD_SSL(ASN1_STRING_PRINT_EX, asn1_string_print_ex_fn, __asn1_string_print_ex);
	LOAD_SSL(BIO_NEW, bio_new_fn, __bio_new);
	LOAD_SSL(BIO_S_MEM, bio_s_mem_fn, __bio_s_mem);
	LOAD_SSL(BIO_CTRL, bio_ctrl_fn, __bio_ctrl);
	LOAD_SSL(BIO_READ, bio_read_fn, __bio_read);
	LOAD_SSL(BIO_FREE, bio_free_fn, __bio_free);
	LOAD_SSL(SSL_CTX_GET0_CERTIFICATE, ssl_ctx_get0_certificate_fn, __ssl_ctx_get0_certificate);
	LOAD_SSL(X509_GET_EXT_D2I, x509_get_ext_d2i_fn, __x509_get_ext_d2i);
	LOAD_SSL(SSL_GET_SERVERNAME, ssl_get_servername_fn, __ssl_get_servername);
	LOAD_SSL(SSL_GET_SSL_CTX, ssl_get_ssl_ctx_fn, __ssl_get_ssl_ctx);
	LOAD_SSL(SSL_SET_SSL_CTX, ssl_set_ssl_ctx_fn, __ssl_set_ssl_ctx);
	LOAD_SSL(SSL_CTX_GET_OPTION, ssl_ctx_get_options_fn, __ssl_ctx_get_options);
	LOAD_SSL(SSL_SET_OPTIONS, ssl_set_options_fn, __ssl_set_options);
	LOAD_SSL(OPENSSL_SK_NUM, openssl_sk_num_fn, __openssl_sk_num);
	LOAD_SSL(OPENSSL_SK_VALUE, openssl_sk_value_fn, __openssl_sk_value);
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

//////////////////////////////////////////////////////////////////////////////

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
#  define __ssl_ctx_use_cert		SSL_CTX_use_certificate_file
#  define __ssl_ctx_use_pkey		SSL_CTX_use_PrivateKey_file
#  define __ssl_ctx_check_pkey		SSL_CTX_check_private_key
#  define __ssl_ctx_set_def_pass	SSL_CTX_set_default_passwd_cb_userdata
#  define __ssl_ctx_set_timeout		SSL_CTX_set_timeout
#  define __ssl_ctx_callback_ctrl	SSL_CTX_callback_ctrl
#  define __ssl_ctx_ctrl		SSL_CTX_ctrl
#  define __asn1_string_print_ex	ASN1_STRING_print_ex
#  define __bio_new			BIO_new
#  define __bio_s_mem			BIO_s_mem
#  define __bio_ctrl			BIO_ctrl
#  define __bio_read			BIO_read 
#  define __bio_free			BIO_free
#  define __ssl_ctx_get0_certificate	SSL_CTX_get0_certificate
#  define __x509_get_ext_d2i		X509_get_ext_d2i
#  define __ssl_get_servername		SSL_get_servername
#  define __ssl_get_ssl_ctx		SSL_get_SSL_CTX
#  define __ssl_set_ssl_ctx		SSL_set_SSL_CTX
#  define __ssl_ctx_get_options		SSL_CTX_get_options
#  define __ssl_set_options		SSL_set_options
#  define __openssl_sk_num		OPENSSL_sk_num
#  define __openssl_sk_value		OPENSSL_sk_value
# endif // !HAS_OPENSSL_DLL

#endif  // HAS_OPENSSL

//////////////////////////////////////////////////////////////////////////////

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

void* openssl_conf::get_libssl_handle(void)
{
#ifdef HAS_OPENSSL_DLL
	return __openssl_ssl_dll;
#else
	return NULL;
#endif
}

void* openssl_conf::get_libcrypto_handle(void)
{
#ifdef HAS_OPENSSL_DLL
	return __openssl_crypto_dll;
#else
	return NULL;
#endif
}

bool openssl_conf::load(void)
{
#ifdef HAS_OPENSSL_DLL
	acl_pthread_once(&__load_once, openssl_dll_load);
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

#ifdef HAS_OPENSSL
static int __once_inited = CONF_INIT_NIL;

static void openssl_once(void)
{
# ifdef HAS_OPENSSL_DLL
	if (!openssl_conf::load()) {
		__once_inited = CONF_INIT_ERR;
		return;
	}
# endif

# if OPENSSL_VERSION_NUMBER >= 0x10100003L
	if (__ssl_init(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
		logger_error("OPENSSL_init_ssl error");
		__once_inited = CONF_INIT_ERR;
	} else {
		// OPENSSL_init_ssl() may leave errors in the error queue
		// while returning success -- nginx
		__ssl_clear_error();
		__once_inited = CONF_INIT_OK;
	}
# else
	__ssl_config(NULL);
	__ssl_library_init();
	__ssl_load_error_strings();
	__ssl_add_all_algorithms();
	__once_inited = CONF_INIT_OK;
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
}
#endif  // HAS_OPENSSL

//////////////////////////////////////////////////////////////////////////////

#ifdef HAS_OPENSSL
static acl_pthread_once_t __openssl_once = ACL_PTHREAD_ONCE_INIT;
#endif

openssl_conf::openssl_conf(bool server_side /* false */, int timeout /* 30 */)
: server_side_(server_side)
, ssl_ctx_(NULL)
, ssl_ctx_table_(NULL)
, timeout_(timeout)
{
#ifdef HAS_OPENSSL
	// Init OpenSSL globally, and the dynamic libs will be loaded
	// automatically when HAS_OPENSSL_DLL has been defined.
	acl_pthread_once(&__openssl_once, openssl_once);
	status_ = __once_inited;

	if (status_ == CONF_INIT_OK) {
		// We shouldn't create ssl_ctx_ in server mode which will
		// be created when loading certificate in add_cert().
		// In client mode, ssl_ctx_ will be set automatically
		// in create_ssl_ctx() for the first calling.
		if (!server_side_ && !(ssl_ctx_ = create_ssl_ctx())) {
			status_  = CONF_INIT_ERR;
		}
	}

	if (status_ == CONF_INIT_ERR) {
		logger_error("Init MbedTLS failed!");
	}
#else
	status_ = CONF_INIT_ERR;
	(void) timeout_;
	logger_error("HAS_OPENSSL not defined!");
#endif // HAS_OPENSSL

}

openssl_conf::~openssl_conf(void)
{
#ifdef HAS_OPENSSL
	for (std::set<SSL_CTX*>::iterator it = ssl_ctxes_.begin();
		it != ssl_ctxes_.end(); ++it) {
		__ssl_ctx_free(*it);

	}

	delete ssl_ctx_table_;
#endif
}

SSL_CTX* openssl_conf::create_ssl_ctx(void)
{
#ifdef HAS_OPENSSL
	if (status_ != CONF_INIT_OK) {
		logger_error("OpenSSL not init , status=%d", (int) status_);
		return NULL;
	}

	SSL_CTX* ctx = __ssl_ctx_new(__sslv23_method());

	if (timeout_ > 0) {
		__ssl_ctx_set_timeout(ctx, timeout_);
	}

	if (server_side_) {
		// SSL_CTX_set_tlsext_servername_callback(ctx, sni_callback);
		// SSL_CTX_set_tlsext_servername_arg(ctx, this);

		__ssl_ctx_callback_ctrl(ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_CB,
				(void (*)(void)) sni_callback);
		__ssl_ctx_ctrl(ctx, SSL_CTRL_SET_TLSEXT_SERVERNAME_ARG, 0, this);
	}

	if (ssl_ctx_ == NULL) {
		ssl_ctx_ = ctx;
	}

	ssl_ctxes_.insert(ctx);
	return ctx;
#else
	return NULL;
#endif
}

bool openssl_conf::push_ssl_ctx(SSL_CTX* ctx)
{
	if (ctx == NULL) {
		return false;
	}

	if (!server_side_) {
		logger_error("Can't be used in client mode!");
		return false;
	}

#ifdef HAS_OPENSSL
	map_ssl_ctx(ctx);
	return true;
#else
	logger_error("HAS_OPENSSL not been set yet!");
	return false;
#endif
}

SSL_CTX* openssl_conf::get_ssl_ctx(void) const
{
	// The ssl_ctx_ should be created in client mode, and in server mode,
	// it should also be created when loading certificate.
	if (ssl_ctx_ == NULL) {
		logger_error("ssl_ctx_ not created yet!");
	}
	return ssl_ctx_;
}

void openssl_conf::get_ssl_ctxes(std::vector<SSL_CTX*>& out)
{
#ifdef HAS_OPENSSL
	for (std::set<SSL_CTX*>::iterator it = ssl_ctxes_.begin();
		it != ssl_ctxes_.end(); ++it) {
		out.push_back(*it);
	}
#else
	(void) out;
#endif
}

#ifdef HAS_OPENSSL
/* convert an ASN.1 string to a UTF-8 string (escaping control characters) */  
static char *asn1_string_to_utf8(ASN1_STRING *asn1str)
{
	char *result = NULL;
	BIO *bio;
	int len;

	if ((bio = __bio_new(__bio_s_mem())) == NULL) {
		return NULL;
	}

	__asn1_string_print_ex(bio, asn1str, ASN1_STRFLGS_ESC_CTRL |
			ASN1_STRFLGS_UTF8_CONVERT);

	// len = BIO_pending(bio);
	// BIO_pending(b) => (int)BIO_ctrl(b,BIO_CTRL_PENDING,0,NULL)
	len = __bio_ctrl(bio, BIO_CTRL_PENDING, 0, NULL);
	if (len > 0) {
		result = (char*) malloc(len + 1);
		len = __bio_read(bio, result, len);
		result[len] = 0;
	}

	__bio_free(bio);
	return result;
}
#endif

void openssl_conf::map_ssl_ctx(SSL_CTX* ctx)
{
	std::vector<string> hosts;
	get_hosts(ctx, hosts);

	for (std::vector<string>::iterator cit = hosts.begin();
		cit != hosts.end(); ++cit) {
		bind_host(ctx, *cit);
	}
}

void openssl_conf::get_hosts(const SSL_CTX* ctx, std::vector<string>& hosts)
{
#ifdef HAS_OPENSSL
	X509* x509 = __ssl_ctx_get0_certificate(ctx);
	if (x509 == NULL) {
		logger_error("SSL_CTX_get0_certificate NULL");
		return;
	}

	STACK_OF(GENERAL_NAME) *names = (struct stack_st_GENERAL_NAME*)
		__x509_get_ext_d2i(x509, NID_subject_alt_name, NULL, NULL);
	if (names == NULL) {
		return;
	}

	//for (int i = 0; i < sk_GENERAL_NAME_num(names); i++) {
	for (int i = 0; i < __openssl_sk_num((const OPENSSL_STACK*) names); i++) {
		//GENERAL_NAME *name = sk_GENERAL_NAME_value(names, i);
		GENERAL_NAME *name = (GENERAL_NAME*)
			__openssl_sk_value((const OPENSSL_STACK*) names, i);
		if (name->type != GEN_DNS) {
			continue;
		}

		char* host = asn1_string_to_utf8(name->d.ia5);
		if (host) {
			//logger(">>>get host %s", host);
			hosts.push_back(host);
			free(host);
		}
	}
#else
	(void) ctx;
	(void) hosts;
#endif
}

void openssl_conf::bind_host(SSL_CTX* ctx, string& host)
{
	string key;
	if (!create_host_key(host, key)) {
		return;
	}

	if (ssl_ctx_table_ == NULL) {
		ssl_ctx_table_ = NEW token_tree;
	}

	if (ssl_ctx_table_->find(key) == NULL) {
		logger("add host=%s, key=%s", host.c_str(), key.c_str());
		ssl_ctx_table_->insert(key, ctx);
	}
}

bool openssl_conf::create_host_key(string& host, string& key, size_t skip /* 0 */)
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

SSL_CTX* openssl_conf::find_ssl_ctx(const char* host)
{
	if (ssl_ctx_table_ == NULL) {
		return NULL;
	}

	string host_buf(host), key;
	if (!create_host_key(host_buf, key)) {
		return NULL;
	}

	//printf(">>>host=%s, key=%s\r\n", host, key.c_str());

	const token_node* node = ssl_ctx_table_->find(key);
	if (node != NULL) {
		SSL_CTX* ctx = (SSL_CTX*) node->get_ctx();
		return ctx;
	}

	// Try the wildcard matching process, and cut off the last item
	// in the host name.
	if (!create_host_key(host_buf, key, 1)) {
		return NULL;
	}

	// The char '.' must be appended in order to wildcard matching,
	// because the wildcard host's lst char must be '.'.
	key += ".";

	node = ssl_ctx_table_->find(key);
	if (node != NULL) {
		SSL_CTX* ctx = (SSL_CTX*) node->get_ctx();
		return ctx;
	}

	//printf("Not found key=%s\r\n", key.c_str());
	return NULL;
}

int openssl_conf::on_sni_callback(SSL* ssl, const char*host)
{
#ifdef HAS_OPENSSL
	SSL_CTX* ctx = find_ssl_ctx(host);
	if (ctx == NULL) {
		return SSL_TLSEXT_ERR_NOACK;
	}

	SSL_CTX* orig;
	if ((orig = __ssl_get_ssl_ctx(ssl)) == ctx) {
		//printf(">>>use default ssl_ctx\n");
		return SSL_TLSEXT_ERR_NOACK;
	}

	if (__ssl_set_ssl_ctx(ssl, ctx) == ctx) {
		__ssl_set_options(ssl, __ssl_ctx_get_options(orig));
		//printf("reset ctx ok\r\n");
		return SSL_TLSEXT_ERR_OK;
	}
	//printf("reset ctx=%p error\r\n", ctx);
	return SSL_TLSEXT_ERR_NOACK;
#else
	(void) ssl;
	(void) host;
	return 0;
#endif
}

int openssl_conf::sni_callback(SSL *ssl, int *ad, void *arg)
{
#ifdef HAS_OPENSSL
	(void) ad;

	openssl_conf* conf = (openssl_conf*) arg;

	if (conf->ssl_ctx_table_ == NULL) {
		return SSL_TLSEXT_ERR_NOACK;
	}

	const char* host = __ssl_get_servername(ssl, TLSEXT_NAMETYPE_host_name);
	if (host == NULL) {
		return SSL_TLSEXT_ERR_NOACK;
	}

	return conf->on_sni_callback(ssl, host);
#else
	(void) ssl;
	(void) ad;
	(void) arg;
	return 0;
#endif
}

bool openssl_conf::load_ca(const char* ca_file, const char* /* ca_path */)
{
	if (status_ != CONF_INIT_OK) {
		logger_error("OpenSSL not init , status=%d", (int) status_);
		return false;
	}

	if (ca_file == NULL) {
		logger_error("ca_file NULL");
		return false;
	}

#ifdef HAS_OPENSSL
	__ssl_ctx_set_verify_depth(ssl_ctx_, 5);

	STACK_OF(X509_NAME)* list = __ssl_load_client_ca(ca_file);
	if (list == NULL) {
		logger_error("load CA file(%s) error", ca_file);
		return false;
	}

	// Before 0.9.7h and 0.9.8 SSL_load_client_CA_file()
	// always leaved an error in the error queue. -- nginx
	__ssl_clear_error();

	__ssl_ctx_set_client_ca(ssl_ctx_, list);
	return true;
#else
	logger_error("HAS_OPENSSL not defined!");
	return false;
#endif
}

bool openssl_conf::add_cert(const char* crt_file, const char* key_file,
	const char* key_pass /* NULL */)
{
	if (status_ != CONF_INIT_OK) {
		logger_error("OpenSSL not init , status=%d", (int) status_);
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

#ifdef HAS_OPENSSL
	SSL_CTX* ctx = create_ssl_ctx();

#if 0
	if (__ssl_ctx_use_cert_chain(ctx, crt_file) != 1) {
		logger_error("use crt chain file(%s) error", crt_file);
		return false;
	}
#else
	if (__ssl_ctx_use_cert(ctx, crt_file, SSL_FILETYPE_PEM) != 1) {
		logger_error("use crt file(%s) error", crt_file);
		return false;
	}
#endif

	if (__ssl_ctx_use_pkey(ctx, key_file, SSL_FILETYPE_PEM) != 1) {
		logger_error("load private key(%s) error", key_file);
		return false;
	}

	if (!__ssl_ctx_check_pkey(ctx)) {
		logger_error("check private key(%s) error", key_file);
		return false;
	}

	if (key_pass && *key_pass) {
		__ssl_ctx_set_def_pass(ctx, (void*) key_pass);
	}

	map_ssl_ctx(ctx);
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
}

sslbase_io* openssl_conf::create(bool nblock)
{
	return NEW openssl_io(*this, server_side_, nblock);
}

} // namespace acl
