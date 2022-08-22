#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include "../stdlib/thread_mutex.hpp"
#include "../stdlib/string.hpp"
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
	 * 调用本函数设置一个动态库的全路径
	 * @param libcrypto {const char*} libcrypto.so 动态库的全路径
	 * @param libssl {const char*} libssl.so 动态库的全路径
	 */
	static void set_libpath(const char* libcrypto, const char* libssl);

	/**
	 * 显式调用本方法，动态加载 libssl.so 动态库
	 * @return {bool} 加载是否成功
	 */
	static bool load(void);

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

	bool   server_side_;
	void*  ssl_ctx_;
	string crt_file_;
	unsigned init_status_;
	thread_mutex lock_;

	bool init_once(void);
};

} // namespace acl
