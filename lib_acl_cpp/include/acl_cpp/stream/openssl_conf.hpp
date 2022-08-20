#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "sslbase_conf.hpp"

namespace acl {

class openssl_io;

class ACL_CPP_API openssl_conf : public sslbase_conf {
public:
	openssl_conf(bool server_side = false);
	~openssl_conf(void);

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
	void enable_cache(bool on);

public:
	// @override sslbase_conf
	sslbase_io* create(bool nblock);

public:
	bool setup_certs(void* ssl);

	bool is_server_side(void) const
	{
		return server_side_;
	}

	void* get_ssl_ctx(void) const
	{
		return ssl_ctx_;
	}

private:
	friend class openssl_io;

	bool  server_side_;
	void* ssl_ctx_;
};

} // namespace acl
