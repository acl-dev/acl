#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class sslbase_io;

class ACL_CPP_API ssl_sni_checker {
public:
	ssl_sni_checker() {}
	virtual ~ssl_sni_checker() {}

	/**
	 * 虚方法用来检查输入的sni host是否合法，子类必须实现
	 * @param sni {const char*} 客户端传来的 sni 字段
	 * @param host {acl::string&} 从 sni 中提取的 host 字段
	 * @return {bool} 检查是否合法
	 */
	virtual bool check(sslbase_io* io, const char* sni, string& host) = 0;
};

class ACL_CPP_API sslbase_conf : public noncopyable {
public:
	sslbase_conf() : checker_(NULL) {}
	virtual ~sslbase_conf() {}

	/**
	 * 纯虚方法，创建 SSL IO 对象
	 * @param nblock {bool} 是否为非阻塞模式
	 * @return {sslbase_io*}
	 */
	virtual sslbase_io* create(bool nblock) = 0;

public:
	/**
	 * 加载 CA 根证书(每个配置实例只需调用一次本方法)
	 * @param ca_file {const char*} CA 证书文件全路径
	 * @param ca_path {const char*} 多个 CA 证书文件所在目录
	 * @return {bool} 加载  CA 根证书是否成功
	 * 注：如果 ca_file、ca_path 均非空，则会依次加载所有证书
	 */
	virtual bool load_ca(const char* ca_file, const char* ca_path) {
		(void) ca_file;
		(void) ca_path;
		return false;
	}

	/**
	 * 添加一个服务端/客户端自己的证书，可以多次调用本方法加载多个证书
	 * @param crt_file {const char*} 证书文件全路径，非空
	 * @param key_file {const char*} 密钥文件全路径，非空
	 * @param key_pass {const char*} 密钥文件的密码，没有密钥密码可写 NULL
	 * @return {bool} 添加证书是否成功
	 */
	virtual bool add_cert(const char* crt_file, const char* key_file,
		const char* key_pass) {
		(void) crt_file;
		(void) key_file;
		(void) key_pass;
		return false;
	}

	// 仅为了兼容旧的API
	bool add_cert(const char* crt_file, const char* key_file) {
		return add_cert(crt_file, key_file, NULL);
	}

	/**
	 * 添加一个服务端/客户端自己的证书，可以多次调用本方法加载多个证书
	 * @param crt_file {const char*} 证书文件全路径，非空
	 * @return {bool} 添加证书是否成功
	 * @deprecated use add_cert(const char*, const char*, const char*)
	 */
	virtual bool add_cert(const char* crt_file) {
		(void) crt_file;
		return false;
	}

	/**
	 * 添加服务端/客户端的密钥(每个配置实例只需调用一次本方法)
	 * @param key_file {const char*} 密钥文件全路径，非空
	 * @param key_pass {const char*} 密钥文件的密码，没有密钥密码可写 NULL
	 * @return {bool} 设置是否成功
	 * @deprecated use add_cert(const char*, const char*, const char*)
	 */
	virtual bool set_key(const char* key_file, const char* key_pass) {
		(void) key_file;
		(void) key_pass;
		return false;
	}

	// 仅为了兼容旧的API
	bool set_key(const char* key_file) {
		return set_key(key_file, NULL);
	}

	/**
	 * 当为服务端模式时是否启用会话缓存功能，有助于提高 SSL 握手效率
	 * @param on {bool}
	 * 注：该函数仅对服务端模式有效
	 */
	virtual void enable_cache(bool on) {
		(void) on;
	}

	/**
	 * 设置客户端发送的 SNI 校验类对象
	 * @param checker {ssl_sni_checker*}
	 */
	void set_sni_checker(ssl_sni_checker* checker) {
		checker_ = checker;
	}

	/**
	 * 获得所设置的 SNI 校验对象
	 * @return {ssl_sni_checker*}
	 */
	ssl_sni_checker* get_sni_checker() const {
		return checker_;
	}

protected:
	ssl_sni_checker* checker_;
};

} // namespace acl
