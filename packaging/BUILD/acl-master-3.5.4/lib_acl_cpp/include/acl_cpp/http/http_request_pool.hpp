#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_pool.hpp"

namespace acl
{

class sslbase_conf;

/**
 * http 客户端连接池类，该类父类为 connect_pool，该类只需实现父类中的虚函数
 * create_connect 便拥有了连接池父类 connect_pool 的功能；另外，该类创建
 * 的连接对象是 http_reuqest 对象，所以在调用 connect_pool::peek 时返回
 * 的便是 http_request 类，调用者需要将 peek 返回的类对象强制转为 http_request
 * 类对象，便可以使用 http_request 类折所有功能，其中 http_reuqest 类为
 * connect_client 的子类
 */
class ACL_CPP_API http_request_pool : public connect_pool
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} 服务器监听地址，格式：ip:port(domain:port)
	 * @param count {size_t} 连接池最大连接个数限制，当该值为 0 时则没有限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 */
	http_request_pool(const char* addr, size_t count, size_t idx = 0);
	~http_request_pool();

	/**
	 * 调用本函数设置 SSL 的客户端模式
	 * @param ssl_conf {sslbase_conf*}
	 */
	void set_ssl(sslbase_conf* ssl_conf);

protected:
	// 基类纯虚函数，该函数返回后由基类设置该连接池的网络连接及网络 IO 超时时间
	virtual connect_client* create_connect();

private:
	sslbase_conf* ssl_conf_;
};

class ACL_CPP_API http_guard : public connect_guard
{
public:
	http_guard(http_request_pool& pool);
	~http_guard(void);
};

} // namespace acl
