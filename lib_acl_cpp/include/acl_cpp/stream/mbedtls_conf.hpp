#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread_mutex.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/token_tree.hpp"
#include "sslbase_conf.hpp"
#include <vector>

typedef struct mbedtls_x509_crt mbedtls_x509_crt;
typedef struct mbedtls_ssl_config  mbedtls_ssl_config;
typedef struct mbedtls_ssl_cache_context mbedtls_ssl_cache_context;
typedef struct mbedtls_ssl_context mbedtls_ssl_context;

namespace acl {

typedef struct MBEDTLS_CERT_KEY {
	mbedtls_x509_crt* cert;
	void* pkey;
} MBEDTLS_CERT_KEY;

/**
 * SSL certificate verification level type definition
 */
typedef enum {
	MBEDTLS_VERIFY_NONE,	// Don't verify certificate
	MBEDTLS_VERIFY_OPT,	// Optional verification, can verify during or after handshake
	MBEDTLS_VERIFY_REQ	// Require verification during handshake
} mbedtls_verify_t;

class mbedtls_io;

/**
 * Configuration class for SSL connection objects. Objects of this class can
 * generally be declared as global objects, used to configure certificates for
 * each SSL
 * connection object. This class loads global certificate, key and other
 * information. Each SSL object
 * (mbedtls_io) calls setup_certs method of this object to initialize its own
 * certificate, key and other information
 */
class ACL_CPP_API mbedtls_conf : public sslbase_conf {
public:
	/**
	 * Constructor
	 * @param server_side {bool} Used to specify whether it is server or client.
	 * When true,
	 *  it is server mode, otherwise client mode
	 * @param verify_mode {mbedtls_verify_t} SSL certificate verification level
	 */
	mbedtls_conf(bool server_side = false,
		mbedtls_verify_t verify_mode = MBEDTLS_VERIFY_NONE);
	~mbedtls_conf();

	/**
	 * @override
	 */
	bool load_ca(const char* ca_file, const char* ca_path);

	/**
	 * @override
	 */
	bool add_cert(const char* crt_file, const char* key_file,
		const char* key_pass = NULL);

	/**
	 * @override
	 * Note: This method will be deprecated in mbedtls_conf, please use the above
	 * method directly
	 */
	bool add_cert(const char* /* crt_file */);

	/**
	 * @override
	 * Note: This method will be deprecated in mbedtls_conf, please use the above
	 * method directly
	 */
	bool set_key(const char* /*key_file*/, const char* /* key_pass */);

	/**
	 * @override
	 */
	void enable_cache(bool on);

public:
	/**
	 * mbedtls_io::open internally will call this method to install certificate for
	 * current SSL connection object
	 * @param ssl {void*} SSL connection object, is ssl_context type
	 * @return {bool} Whether configuring SSL object was successful
	 */
	bool setup_certs(void* ssl);

	/**
	 * Get entropy object of random number generator
	 * @return {void*}, return value is entropy_context type
	 */
	void* get_entropy() const {
		return entropy_;
	}

public:
	/**
	 * If mbedtls is split into three libraries, can call this function to set full
	 * paths of three dynamic libraries
	 * @param libmbedcrypto {const char*} Full path of libmbedcrypto dynamic
	 * library
	 * @param libmbedx509 {const char*} Full path of libmbedx509 dynamic library
	 * @param libmbedtls {const char*} Full path of libmbedtls dynamic library
	 */
	static void set_libpath(const char* libmbedcrypto,
		const char* libmbedx509, const char* libmbedtls);

	/**
	 * If mbedtls is combined into one library, can call this function to set full
	 * path of one dynamic library
	 * @param libmbedtls {const char*} Full path of libmbedtls dynamic library
	 */
	static void set_libpath(const char* libmbedtls);

	/**
	 * Explicitly call this method to dynamically load mbedtls dynamic library
	 * @return {bool} Whether loading was successful
	 */
	static bool load();

public:
	// @override sslbase_conf
	sslbase_io* create(bool nblock);

	// @override sslbase_conf
	bool set_version(int ver_min, int ver_max);

public:
	mbedtls_ssl_config* create_ssl_config();

private:
	unsigned status_;
	bool  server_side_;

	token_tree* conf_table_;
	mbedtls_ssl_config* conf_;
	std::set<mbedtls_ssl_config*> certs_;

	const int* ciphers_;
	void* entropy_;
	void* rnd_;

	mbedtls_x509_crt* cacert_;
	string crt_file_;
	mbedtls_ssl_cache_context* cache_;
	mbedtls_verify_t verify_mode_;

	std::vector<MBEDTLS_CERT_KEY*> cert_keys_;

	bool create_host_key(string& host, string& key, size_t skip = 0);
	void get_hosts(const mbedtls_x509_crt& cert, std::vector<string>& hosts);
	void bind_host(string& host, MBEDTLS_CERT_KEY* ck);

	void map_cert(const mbedtls_x509_crt& cert, MBEDTLS_CERT_KEY* ck);
	MBEDTLS_CERT_KEY* find_ssl_config(const char* host);

private:
	int on_sni_callback(mbedtls_ssl_context* ssl,
		const unsigned char* name, size_t name_len);
	static int sni_callback(void* arg, mbedtls_ssl_context* ssl,
		const unsigned char* name, size_t name_len);

private:
	bool init_rand();
	void free_ca();
};

} // namespace acl

