#include "acl_stdafx.hpp"
#ifdef HAS_POLARSSL
# include "polarssl/ssl.h"
# include "polarssl/entropy.h"
# include "polarssl/certs.h"
# include "polarssl/x509_crt.h"
# include "polarssl/ssl_cache.h"
#endif
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/polarssl_conf.hpp"

namespace acl
{

polarssl_conf::polarssl_conf()
{
#ifdef HAS_POLARSSL
	entropy_ = acl_mycalloc(1, sizeof(entropy_context));
	::entropy_init((entropy_context*) entropy_);

	cacert_ = NULL;
	cert_chain_ = NULL;
	pkey_ = NULL;
	
	cache_ = NULL;
#endif
}

polarssl_conf::~polarssl_conf()
{
#ifdef HAS_POLARSSL
	::entropy_free((entropy_context*) entropy_);
	acl_myfree(entropy_);

	free_ca();

	if (cert_chain_)
	{
		::x509_crt_free((x509_crt*) cert_chain_);
		acl_myfree(cert_chain_);
	}

	if (pkey_)
	{
		::pk_free((pk_context*) pkey_);
		acl_myfree(pkey_);
	}

	if (cache_)
	{
		::ssl_cache_free((ssl_cache_context*) cache_);
		acl_myfree(cache_);
	}
#endif
}

void polarssl_conf::free_ca()
{
#ifdef HAS_POLARSSL
	if (cacert_)
	{
		::x509_crt_free((x509_crt*) cacert_);
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
	cacert_ = acl_mycalloc(1, sizeof(x509_crt));
	::x509_crt_init((x509_crt*) cacert_);

	if (ca_path && *ca_path)
	{
		ret = ::x509_crt_parse_path((x509_crt*) cacert_, ca_path);
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

	ret = ::x509_crt_parse_file((x509_crt*) cacert_, ca_file);
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
		cert_chain_ = acl_mycalloc(1, sizeof(x509_crt));
		::x509_crt_init((x509_crt*) cert_chain_);
	}

	int   ret = ::x509_crt_parse_file((x509_crt*) cert_chain_, crt_file);
	if (ret != 0)
	{
		logger_error("x509_crt_parse_file(%s) error: -0x%04x",
			crt_file, ret);

		::x509_crt_free((x509_crt*) cert_chain_);
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
		::pk_free((pk_context*) pkey_);
		acl_myfree(pkey_);
	}

	pkey_ = acl_mycalloc(1, sizeof(pk_context));
	::pk_init((pk_context*) pkey_);

	int   ret = ::pk_parse_keyfile((pk_context*) pkey_, key_file,
				key_pass ? key_pass : "");
	if (ret != 0)
	{
		logger_error("pk_parse_keyfile(%s) error: -0x%04x",
			key_file, ret);

		::pk_free((pk_context*) pkey_);
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

void polarssl_conf::enable_cache(bool on)
{
#ifdef HAS_POLARSSL
	if (on)
	{
		if (cache_ != NULL)
			return;
		cache_ = acl_mycalloc(1, sizeof(ssl_cache_context));
		::ssl_cache_init((ssl_cache_context*) cache_);
	}
	else if (cache_ != NULL)
	{
		::ssl_cache_free((ssl_cache_context*) cache_);
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

	::ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_NONE);

	// Setup cipher_suites
	const int* cipher_suites = ::ssl_list_ciphersuites();
	if (cipher_suites == NULL)
	{
		logger_error("ssl_list_ciphersuites null");
		return false;
	}
	::ssl_set_ciphersuites((ssl_context*) ssl, cipher_suites);

	// Setup cache only for server-side
	if (server_side && cache_ != NULL)
		::ssl_set_session_cache(ssl, ssl_cache_get,
			(ssl_cache_context*) cache_, ssl_cache_set,
			(ssl_cache_context*) cache_);

	// Setup ca cert
	if (cacert_)
		::ssl_set_ca_chain(ssl, (x509_crt*) cacert_, NULL, NULL);

	// Setup own's cert chain and private key

	if (cert_chain_ && pkey_)
	{
		int ret = ::ssl_set_own_cert(ssl, (x509_crt*) cert_chain_,
				(pk_context*) pkey_);
		if (ret != 0)
		{
			logger_error("ssl_set_own_cert error: -0x%04x", ret);
			return false;
		}
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
