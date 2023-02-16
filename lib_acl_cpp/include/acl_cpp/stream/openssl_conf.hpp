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
	openssl_conf(bool server_side = false, int timeout = 30);
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

	/**
	 * 调用 load() 成功加载 OpenSSL 动态库后，调用本静态函数获得 libssl
	 * 动态加载库句柄，从而可以从该句柄中获得指定函数指针
	 * @return {void*} 返回 NULL 表示还未加载
	 */
	static void* get_libssl_handle(void);

	/**
	 * 获得 libcrypto 动态加载库句柄
	 * @return {void*} 返回 NULL 表示还未加载
	 */
	static void* get_libcrypto_handle(void);

public:
	// @override sslbase_conf
	sslbase_io* create(bool nblock);

public:
	/**
	 * 是否为 SSL 服务模式
	 * @return {bool}
	 */
	bool is_server_side(void) const {
		return server_side_;
	}

	/**
	 * 获得缺省的SSL_CTX对象
	 * @return {SSL_CTX*}
	 */
	SSL_CTX* get_ssl_ctx(void) const;

	/**
	 * 获得所有的已经初始完成的 SSL_CTX 对象
	 * @param out {std::vector<SSL_CTX*>&}
	 */
	void get_ssl_ctxes(std::vector<SSL_CTX*>& out);

	/**
	 * 服务模式下,创建 SSL_CTX 对象,内部自动设置 SNI 回调过程,虽然内部也是
	 * 通过调用 SSL_CTX_new() API 创建 SSL_CTX 对象,但内部会自动区分动态
	 * 加载或静态加载的 SSL_CTX_new() API.
	 * @return {SSL_CTX*} 返回 NULL 表示未开启 OpenSSL 功能
	 */
	SSL_CTX* create_ssl_ctx(void);

	/**
	 * 服务模式下, 添加外部已经初始完毕的 SSL_CTX, 该对象必须是由上面
	 * create_ssl_ctx() 创建的,以适配动态或静态加载 OpenSSL 的不同方式.
	 * @param {SSL_CTX*} 由用户自自己初始化好的 SSL_CTX 对象，传入后其所有
	 *  权将归 openssl_conf 内部统一管理并释放
	 * @return {bool} 返回 false 表示添加失败，原因可能是 ctx 为 NULL，或
	 *  当前 openssl_conf 对象为客户端模式
	 */
	bool push_ssl_ctx(SSL_CTX* ctx);

private:
	bool         server_side_;
	SSL_CTX*     ssl_ctx_;		// The default SSL_CTX.
	token_tree*  ssl_ctx_table_;	// Holding the map of host/SSL_CTX.
	std::set<SSL_CTX*> ssl_ctxes_;	// Holding all ctx just for freeing.
	int          timeout_;
	string       crt_file_;
	unsigned     status_;

	void map_ssl_ctx(SSL_CTX* ctx);
	SSL_CTX* find_ssl_ctx(const char* host);

	void get_hosts(const SSL_CTX* ctx, std::vector<string>& hosts);
	void bind_host(SSL_CTX* ctx, string& host);
	bool create_host_key(string& host, string& key, size_t skip = 0);

	int on_sni_callback(SSL* ssl, const char*host);
	static int sni_callback(SSL *ssl, int *ad, void *arg);
};

} // namespace acl
