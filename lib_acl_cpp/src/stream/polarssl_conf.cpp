#include "acl_stdafx.hpp"
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

namespace acl
{

#ifdef HAS_POLARSSL
# ifdef POLARSSL_1_3_X
#  define _X509_CERT		x509_crt
#  define _X509_FREE_FN		::x509_crt_free
#  define _X509_INIT_FN		::x509_crt_init
#  define _X509_PARSE_PATH_FN	::x509_crt_parse_path
#  define _X509_PARSE_FILE_FN	::x509_crt_parse_file
#  define _PKEY			pk_context
#  define _PARSE_KEYFILE_FN	::pk_parse_keyfile
#  define _PKEY_FREE_FN		::pk_free
#  define _ENTROPY_FREE_FN		::entropy_free
# else
#  define _X509_CERT		x509_cert
#  define _X509_FREE_FN		::x509_free
#  define _X509_INIT_FN		(void)
#  define _X509_PARSE_PATH_FN	::x509parse_crtpath
#  define _X509_PARSE_FILE_FN	::x509parse_crtfile
#  define _PKEY			rsa_context
#  define _PARSE_KEYFILE_FN	::x509parse_keyfile
#  define _PKEY_FREE_FN		::rsa_free
#  define _ENTROPY_FREE_FN	(void)
# endif
#endif

polarssl_conf::polarssl_conf()
{
#ifdef HAS_POLARSSL
	entropy_ = acl_mycalloc(1, sizeof(entropy_context));
	::entropy_init((entropy_context*) entropy_);

	cacert_ = NULL;
	cert_chain_ = NULL;
	
	cache_ = NULL;
	pkey_ = NULL;
	verify_mode_ = POLARSSL_VERIFY_NONE;
#endif
}

polarssl_conf::~polarssl_conf()
{
#ifdef HAS_POLARSSL
	free_ca();

	if (cert_chain_)
	{
		_X509_FREE_FN((_X509_CERT*) cert_chain_);
		acl_myfree(cert_chain_);
	}


	if (pkey_)
	{
		_PKEY_FREE_FN((_PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	_ENTROPY_FREE_FN((entropy_context*) entropy_);
	acl_myfree(entropy_);

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
	if (cacert_) {
		_X509_FREE_FN((_X509_CERT*) cacert_);
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

	cacert_ = acl_mycalloc(1, sizeof(_X509_CERT));
	_X509_INIT_FN((_X509_CERT*) cacert_);

	if (ca_path && *ca_path)
	{
		ret = _X509_PARSE_PATH_FN((_X509_CERT*) cacert_, ca_path);
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

	ret = _X509_PARSE_FILE_FN((_X509_CERT*) cacert_, ca_file);
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
		cert_chain_ = acl_mycalloc(1, sizeof(_X509_CERT));
		_X509_INIT_FN((_X509_CERT*) cert_chain_);
	}

	int ret = _X509_PARSE_FILE_FN((_X509_CERT*) cert_chain_, crt_file);
	if (ret != 0)
	{
		logger_error("x509_crt_parse_file(%s) error: -0x%04x",
			crt_file, ret);

		_X509_FREE_FN((_X509_CERT*) cert_chain_);
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
		_PKEY_FREE_FN((_PKEY*) pkey_);
		acl_myfree(pkey_);
	}

	pkey_ = acl_mycalloc(1, sizeof(_PKEY));

# ifdef POLARSSL_1_3_X
	::pk_init((_PKEY*) pkey_);
# else
	::rsa_init((_PKEY*) pkey_, RSA_PKCS_V15, 0);
# endif

	int ret = _PARSE_KEYFILE_FN((_PKEY*) pkey_, key_file,
				key_pass ? key_pass : "");
	if (ret != 0)
	{
		logger_error("pk_parse_keyfile(%s) error: -0x%04x",
			key_file, ret);

		_PKEY_FREE_FN((_PKEY*) pkey_);
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

	switch (verify_mode_)
	{
	case POLARSSL_VERIFY_NONE:
		::ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_NONE);
		break;
	case POLARSSL_VERIFY_OPT:
		::ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_OPTIONAL);
		break;
	case POLARSSL_VERIFY_REQ:
		::ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_REQUIRED);
		break;
	default:
		::ssl_set_authmode((ssl_context*) ssl, SSL_VERIFY_NONE);
		break;
	}

	// Setup cipher_suites
	const int* cipher_suites = ::ssl_list_ciphersuites();
	if (cipher_suites == NULL)
	{
		logger_error("ssl_list_ciphersuites null");
		return false;
	}
	::ssl_set_ciphersuites((ssl_context*) ssl, cipher_suites);

//	::ssl_set_min_version((ssl_context*) ssl, SSL_MAJOR_VERSION_3,
//		SSL_MINOR_VERSION_0);
//	::ssl_set_renegotiation((ssl_context*) ssl, SSL_RENEGOTIATION_DISABLED);
//	::ssl_set_dh_param((ssl_context*) &ssl, POLARSSL_DHM_RFC5114_MODP_2048_P,
//		POLARSSL_DHM_RFC5114_MODP_2048_G );

	// Setup cache only for server-side
	if (server_side && cache_ != NULL)
		::ssl_set_session_cache(ssl, ssl_cache_get,
			(ssl_cache_context*) cache_, ssl_cache_set,
			(ssl_cache_context*) cache_);

	// Setup ca cert
	if (cacert_)
		::ssl_set_ca_chain(ssl, (_X509_CERT*) cacert_, NULL, NULL);

	// Setup own's cert chain and private key

	if (cert_chain_ && pkey_)
	{
# ifdef POLARSSL_1_3_X
		int ret = ::ssl_set_own_cert(ssl, (_X509_CERT*) cert_chain_,
				(_PKEY*) pkey_);
		if (ret != 0)
		{
			logger_error("ssl_set_own_cert error: -0x%04x", ret);
			return false;
		}
# else
		::ssl_set_own_cert(ssl, (_X509_CERT*) cert_chain_,
			(_PKEY*) pkey_);
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
