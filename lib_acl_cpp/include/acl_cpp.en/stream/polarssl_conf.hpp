#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread_mutex.hpp"
#include "sslbase_conf.hpp"
#include <vector>

namespace acl {

/**
 * SSL certificate verification level type definition
 */
typedef enum {
	POLARSSL_VERIFY_NONE,	// Don't verify certificate
	POLARSSL_VERIFY_OPT,	// Optional verification, can verify during or after handshake
	POLARSSL_VERIFY_REQ	// Require verification during handshake
} polarssl_verify_t;

class polarssl_io;

/**
 * Configuration class for SSL connection objects. Objects of this class can
 * generally be declared as global objects, used to configure certificates for
 * each SSL
 * connection object. This class loads global certificate, key and other
 * information; each SSL object
 * (polarssl_io) calls setup_certs method of this object to initialize its own
 * certificate, key and other information
 */
class ACL_CPP_API polarssl_conf : public sslbase_conf {
public:
	/**
	 * Constructor
	 * @param server_side {bool} Used to specify whether it is server or client.
	 * When true,
	 *  it is server mode, otherwise client mode
	 * @param verify_mode {polarssl_verify_t} SSL certificate verification level
	 */
	polarssl_conf(bool server_side = false,
		polarssl_verify_t verify_mode = POLARSSL_VERIFY_NONE);
	virtual ~polarssl_conf(void);

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
	 */
	bool add_cert(const char* crt_file);

	/**
	 * @override
	 */
	bool set_key(const char* key_file, const char* key_pass = NULL);

	/**
	 * @override
	 */
	void enable_cache(bool on);

public:
	/**
	 * Set SSL certificate verification method, internal default is not to verify
	 * certificate
	 * @param verify_mode {polarssl_verify_t}
	 */
	void set_authmode(polarssl_verify_t verify_mode);

	/**
	 * Get entropy object of random number generator
	 * @return {void*}, return value is entropy_context type
	 */
	void* get_entropy() {
		return entropy_;
	}

	/**
	 * stream_hook::open internally will call this method to install certificate
	 * for current SSL connection object
	 * @param ssl {void*} SSL connection object, is ssl_context type
	 * @param server_side {bool} Whether it is server or client
	 * @return {bool} Whether configuring SSL object was successful
	 */
	bool setup_certs(void* ssl, bool server_side);

public:
	/**
	 * Must first call this function to set full path of libpolarssl.so
	 * @param path {const char*} Full path of libpolarssl.so
	 */
	static void set_libpath(const char* path);

	/**
	 * Can explicitly call this method to dynamically load polarssl dynamic library
	 * @return {bool} Whether loading was successful
	 */
	static bool load();

public:
	// @override sslbase_conf
	sslbase_io* create(bool nblock);

private:
	friend class polarssl_io;

	bool has_inited_;
	thread_mutex lock_;

	bool  server_side_;
	void* entropy_;
	void* cacert_;
	void* pkey_;
	void* cert_chain_;
	void* cache_;
	polarssl_verify_t verify_mode_;

private:
	void init_once();
	void free_ca();
};

} // namespace acl

