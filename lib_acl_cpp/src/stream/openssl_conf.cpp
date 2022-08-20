#include "acl_stdafx.hpp"

#ifdef HAS_OPENSSL_DLL
# ifndef HAS_OPENSSL
#  define HAS_OPENSSL
# endif
#endif

//#ifndef ACL_PREPARE_COMPILE
# include "acl_cpp/stdlib/log.hpp"
# include "acl_cpp/stream/openssl_io.hpp"
# include "acl_cpp/stream/openssl_conf.hpp"
//#endif

#ifdef HAS_OPENSSL
#include <openssl/ssl.h>
#include <openssl/ssl.h>
#endif
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace acl {

static void init_openssl_once(void)
{
#if OPENSSL_VERSION_NUMBER >= 0x10100003L
	if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CONFIG, NULL) == 0) {
		logger_fatal("OPENSSL_init_ssl error");
	}

	// OPENSSL_init_ssl() may leave errors in the error queue
	// while returning success -- nginx
	ERR_clear_error();
#else
	OPENSSL_config(NULL);
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
#endif

#if OPENSSL_VERSION_NUMBER >= 0x0090800fL
#ifndef SSL_OP_NO_COMPRESSION
	// Disable gzip compression in OpenSSL prior to 1.0.0 version,
	// this saves about 522K per connection. -- nginx
	STACK_OF(SSL_COMP)* ssl_comp_methods = SSL_COMP_get_compression_methods();
	int n = sk_SSL_COMP_num(ssl_comp_methods);
	while (n--) {
		(void) sk_SSL_COMP_pop(ssl_comp_methods);
	}
#endif
#endif
}

openssl_conf::openssl_conf(bool server_side /* false */)
{
	server_side_ = server_side;
	init_openssl_once();
	ssl_ctx_ = (void*) SSL_CTX_new(SSLv23_method());
}

openssl_conf::~openssl_conf(void)
{
	SSL_CTX_free((SSL_CTX *) ssl_ctx_);
}

bool openssl_conf::load_ca(const char* ca_file, const char* /* ca_path */)
{
	if (ca_file == NULL) {
		logger_error("ca_file NULL");
		return false;
	}

	SSL_CTX_set_verify_depth((SSL_CTX*) ssl_ctx_, 5);

	STACK_OF(X509_NAME)* list = SSL_load_client_CA_file(ca_file);
	if (list == NULL) {
		logger_error("load CA file(%s) error", ca_file);
		return false;
	}

	// Before 0.9.7h and 0.9.8 SSL_load_client_CA_file()
	// always leaved an error in the error queue. -- nginx
	ERR_clear_error();

	SSL_CTX_set_client_CA_list((SSL_CTX*) ssl_ctx_, list);
	return true;
}

bool openssl_conf::append_key_cert(const char* crt_file, const char* key_file,
	const char* key_pass /* NULL */)
{
	if (crt_file == NULL || key_file == NULL) {
		return false;
	}

	SSL_CTX* ssl_ctx = (SSL_CTX*) ssl_ctx_;

	if (!SSL_CTX_use_certificate_chain_file(ssl_ctx, crt_file)) {
		logger_error("load crt file(%s) error", crt_file);
		return false;
	}

	if (!SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM)) {
		logger_error("load private key(%s) error", key_file);
		return false;
	}

	if (!SSL_CTX_check_private_key(ssl_ctx)) {
		logger_error("check private key(%s) error", key_file);
		return false;
	}

	if (key_pass && *key_pass) {
		SSL_CTX_set_default_passwd_cb_userdata(ssl_ctx, (void*) key_pass);
	}

	return true;
}

void openssl_conf::enable_cache(bool on)
{
	(void) on;
}

bool openssl_conf::setup_certs(void* ssl)
{
	(void) ssl;
	return true;
}

sslbase_io* openssl_conf::create(bool nblock)
{
	return NEW openssl_io(*this, server_side_, nblock);
}

} // namespace acl
