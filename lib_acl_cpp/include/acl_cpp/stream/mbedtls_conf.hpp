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
 * SSL 证书校验级别类型定义
 */
typedef enum {
	MBEDTLS_VERIFY_NONE,	// 不校验证书
	MBEDTLS_VERIFY_OPT,	// 选择性校验，可以在握手时或握手后校验
	MBEDTLS_VERIFY_REQ	// 要求在握手时校验
} mbedtls_verify_t;

class mbedtls_io;

/**
 * SSL 连接对象的配置类，该类对象一般可以声明为全局对象，用来对每一个 SSL
 * 连接对象进行证书配置；该类加载了全局性的证书、密钥等信息；每一个 SSL 对象
 * (mbedtls_io) 调用本对象的setup_certs 方法来初始化自身的证书、密钥等信息
 */
class ACL_CPP_API mbedtls_conf : public sslbase_conf {
public:
	/**
	 * 构造函数
	 * @param server_side {bool} 用来指定是服务端还是客户端，当为 true 时
	 *  为服务端模式，否则为客户端模式
	 * @param verify_mode {mbedtls_verify_t} SSL 证书校验级别
	 */
	mbedtls_conf(bool server_side = false,
		mbedtls_verify_t verify_mode = MBEDTLS_VERIFY_NONE);
	~mbedtls_conf(void);

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
	 * 注: 该方法在 mbedtls_conf 中将被废弃, 请直接使用上面方法
	 */
	bool add_cert(const char* /* crt_file */);

	/**
	 * @override
	 * 注: 该方法在 mbedtls_conf 中将被废弃, 请直接使用上面方法
	 */
	bool set_key(const char* /*key_file*/, const char* /* key_pass */);

	/**
	 * @override
	 */
	void enable_cache(bool on);

public:
	/**
	 * mbedtls_io::open 内部会调用本方法用来安装当前 SSL 连接对象的证书
	 * @param ssl {void*} SSL 连接对象，为 ssl_context 类型
	 * @return {bool} 配置 SSL 对象是否成功
	 */
	bool setup_certs(void* ssl);

	/**
	 * 获得随机数生成器的熵对象
	 * @return {void*}，返回值为 entropy_context 类型
	 */
	void* get_entropy(void) const {
		return entropy_;
	}

public:
	/**
	 * 如果 mbedtls 分成三个库，可以调用本函数设置三个动态库的全路径
	 * @param libmbedcrypto {const char*} libmbedcrypto 动态库的全路径
	 * @param libmbedx509 {const char*} libmbedx509 动态库的全路径
	 * @param libmbedtls {const char*} libmbedtls 动态库的全路径
	 */
	static void set_libpath(const char* libmbedcrypto,
		const char* libmbedx509, const char* libmbedtls);

	/**
	 * 如果 mbedtls 合成一个库，可以调用本函数设置一个动态库的全路径
	 * @param libmbedtls {const char*} libmbedtls 动态库的全路径
	 */
	static void set_libpath(const char* libmbedtls);

	/**
	 * 显式调用本方法，动态加载 mbedtls 动态库
	 * @return {bool} 加载是否成功
	 */
	static bool load(void);

public:
	// @override sslbase_conf
	sslbase_io* create(bool nblock);

public:
	mbedtls_ssl_config* create_ssl_config(void);

private:
	unsigned status_;
	bool  server_side_;

	int         conf_count_;
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
	bool init_rand(void);
	void free_ca(void);
};

} // namespace acl
