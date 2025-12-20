#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <set>
#include "../stdlib/string.hpp"
#include "../stdlib/token_tree.hpp"
#include "sslbase_conf.hpp"

typedef struct ssl_st SSL;
typedef struct ssl_ctx_st SSL_CTX;

namespace acl {

class token_tree;
class openssl_io;

class ACL_CPP_API openssl_conf : public sslbase_conf {
public:
	explicit openssl_conf(bool server_side = false, int timeout = 30);
	~openssl_conf();

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
	 * @deprecate use add_cert(const char*, const char*, const char*)
	 */
	bool add_cert(const char* crt_file);

	/**
	 * @override
	 * @deprecate use add_cert(const char*, const char*, const char*)
	 */
	bool set_key(const char* key_file, const char* key_pass);

	/**
	 * @override
	 */
	void enable_cache(bool on);

public:
	/**
	 * Call this function to set full path of a dynamic library
	 * @param libcrypto {const char*} Full path of libcrypto.so dynamic library
	 * @param libssl {const char*} Full path of libssl.so dynamic library
	 */
	static void set_libpath(const char* libcrypto, const char* libssl);

	/**
	 * Explicitly call this method to dynamically load libssl.so dynamic library
	 * @return {bool} Whether loading was successful
	 */
	static bool load();

	/**
	 * After successfully loading OpenSSL dynamic library by calling load(), call
	 * this static function to get libssl
	 * dynamically loaded library handle, so that function pointers can be obtained
	 * from this handle
	 * @return {void*} Returns NULL if not yet loaded
	 */
	static void* get_libssl_handle();

	/**
	 * Get libcrypto dynamically loaded library handle
	 * @return {void*} Returns NULL if not yet loaded
	 */
	static void* get_libcrypto_handle();

public:
	// @override sslbase_conf
	sslbase_io* create(bool nblock);

	// @override sslbase_conf
	bool set_version(int ver_min, int ver_max);

	// Bind io with ssl.
	static void bind(SSL* ssl, openssl_io* io);

public:
	/**
	 * Whether it is SSL server mode
	 * @return {bool}
	 */
	bool is_server_side() const {
		return server_side_;
	}

	/**
	 * Get default SSL_CTX object
	 * @return {SSL_CTX*}
	 */
	SSL_CTX* get_ssl_ctx() const;

	/**
	 * Get all SSL_CTX objects that have been initialized
	 * @param out {std::vector<SSL_CTX*>&}
	 */
	void get_ssl_ctxes(std::vector<SSL_CTX*>& out);

	/**
	 * In server mode, create SSL_CTX object. Internally automatically sets SNI
	 * callback process. Although internally also
	 * creates SSL_CTX object by calling SSL_CTX_new() API, internally will
	 * automatically distinguish between dynamically
	 * loaded or statically loaded SSL_CTX_new() API.
	 * @return {SSL_CTX*} Returns NULL indicates OpenSSL functionality is not
	 * enabled
	 */
	SSL_CTX* create_ssl_ctx();

	/**
	 * In server mode, add externally initialized SSL_CTX. This object must be
	 * created by the above
	 * create_ssl_ctx() to adapt to different ways of dynamically or statically
	 * loading OpenSSL.
	 * @param {SSL_CTX*} SSL_CTX object initialized by user, after passing in, its
	 * ownership
	 *  will be managed and released internally by openssl_conf
	 * @return {bool} Returns false indicates add failed, reason may be ctx is
	 * NULL, or
	 *  current openssl_conf object is client mode
	 */
	bool push_ssl_ctx(SSL_CTX* ctx);

	/**
	 * When setting read/write timeout, whether to use setsockopt()
	 * @param yes {bool} If true, use setsockopt to set read/write timeout,
	 * otherwise
	 * use acl_read_wait/acl_write_wait to check timeout situation. Internal
	 * default value is true.
	 */
	void use_sockopt_timeout(bool yes);

	/**
	 * Whether to use setsockopt() to set network timeout.
	 * @return {bool}
	 */
	bool is_sockopt_timeout() const {
		return sockopt_timeout_;
	}

private:
	bool         server_side_;
	SSL_CTX*     ssl_ctx_;		// The default SSL_CTX.
	token_tree*  ssl_ctx_table_;	// Holding the map of host/SSL_CTX.
	std::set<SSL_CTX*> ssl_ctxes_;	// Holding all ctx just for freeing.
	int          timeout_;
	bool         sockopt_timeout_;
	string       crt_file_;
	unsigned     status_;

	void map_ssl_ctx(SSL_CTX* ctx);
	SSL_CTX* find_ssl_ctx(const char* host);

	void get_hosts(const SSL_CTX* ctx, std::vector<string>& hosts);
	void bind_host(SSL_CTX* ctx, string& host);
	bool create_host_key(string& host, string& key, size_t skip = 0);

	int on_sni_callback(SSL* ssl);
	static int sni_callback(SSL *ssl, int *ad, void *arg);
};

} // namespace acl

