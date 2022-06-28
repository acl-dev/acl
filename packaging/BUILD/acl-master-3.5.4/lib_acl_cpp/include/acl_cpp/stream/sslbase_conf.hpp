#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl
{

class sslbase_io;

class ACL_CPP_API sslbase_conf : public noncopyable
{
public:
	sslbase_conf(void) {}
	virtual ~sslbase_conf(void) {}

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
	virtual bool load_ca(const char* ca_file, const char* ca_path)
	{
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
	virtual bool append_key_cert(const char* crt_file, const char* key_file,
		const char* key_pass = NULL)
	{
		(void) crt_file;
		(void) key_file;
		(void) key_pass;
		return false;
	}

	/**
	 * 添加一个服务端/客户端自己的证书，可以多次调用本方法加载多个证书
	 * @param crt_file {const char*} 证书文件全路径，非空
	 * @return {bool} 添加证书是否成功
	 * @deprecated use append_key_cert
	 */
	virtual bool add_cert(const char* crt_file)
	{
		(void) crt_file;
		return false;
	}

	/**
	 * 添加服务端/客户端的密钥(每个配置实例只需调用一次本方法)
	 * @param key_file {const char*} 密钥文件全路径，非空
	 * @param key_pass {const char*} 密钥文件的密码，没有密钥密码可写 NULL
	 * @return {bool} 设置是否成功
	 * @deprecated use append_key_cert
	 */
	virtual bool set_key(const char* key_file, const char* key_pass = NULL)
	{
		(void) key_file;
		(void) key_pass;
		return false;
	}

	/**
	 * 当为服务端模式时是否启用会话缓存功能，有助于提高 SSL 握手效率
	 * @param on {bool}
	 * 注：该函数仅对服务端模式有效
	 */
	virtual void enable_cache(bool on)
	{
		(void) on;
	}
};

} // namespace acl
