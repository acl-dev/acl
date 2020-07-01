#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread_mutex.hpp"
#include "sslbase_conf.hpp"
#include <vector>

namespace acl
{

/**
 * SSL 证书校验级别类型定义
 */
typedef enum
{
	POLARSSL_VERIFY_NONE,	// 不校验证书
	POLARSSL_VERIFY_OPT,	// 选择性校验，可以在握手时或握手后校验
	POLARSSL_VERIFY_REQ	// 要求在握手时校验
} polarssl_verify_t;

class polarssl_io;

/**
 * SSL 连接对象的配置类，该类对象一般可以声明为全局对象，用来对每一个 SSL
 * 连接对象进行证书配置；该类加载了全局性的证书、密钥等信息；每一个 SSL 对象
 * (polarssl_io) 调用本对象的setup_certs 方法来初始化自身的证书、密钥等信息
 */
class ACL_CPP_API polarssl_conf : public sslbase_conf
{
public:
	/**
	 * 构造函数
	 * @param server_side {bool} 用来指定是服务端还是客户端，当为 true 时
	 *  为服务端模式，否则为客户端模式
	 * @param verify_mode {polarssl_verify_t} SSL 证书校验级别
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
	 * 设置 SSL 证书校验方式，内部缺省为不校验证书
	 * @param verify_mode {polarssl_verify_t}
	 */
	void set_authmode(polarssl_verify_t verify_mode);

	/**
	 * 获得随机数生成器的熵对象
	 * @return {void*}，返回值为 entropy_context 类型
	 */
	void* get_entropy(void)
	{
		return entropy_;
	}

	/**
	 * stream_hook::open 内部会调用本方法用来安装当前 SSL 连接对象的证书
	 * @param ssl {void*} SSL 连接对象，为 ssl_context 类型
	 * @param server_side {bool} 是服务端还是客户端
	 * @return {bool} 配置 SSL 对象是否成功
	 */
	bool setup_certs(void* ssl, bool server_side);

public:
	/**
	 * 必须首先调用此函数设置 libpolarssl.so 的全路径
	 * @param path {const char*} libpolarssl.so 的全路径
	 */
	static void set_libpath(const char* path);

	/**
	 * 可以显式调用本方法，动态加载 polarssl 动态库
	 * @return {bool} 加载是否成功
	 */
	static bool load(void);

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
	void init_once(void);
	void free_ca(void);
};

} // namespace acl
