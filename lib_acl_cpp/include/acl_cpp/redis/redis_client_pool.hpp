#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_pool.hpp"

namespace acl
{

/**
 * redis 连接池类，该类继承于 connect_pool，在 connect_pool 定义了通用的有关
 * TCP 连接池的通用方法。
 * redis connection pool inherting from connect_pool, which includes
 * TCP connection pool methods.
 */
class ACL_CPP_API redis_client_pool : public connect_pool
{
public:
	/**
	 * 构造函数
	 * constructor
	 * @param addr {const char*} 服务端地址，格式：ip:port
	 *  the redis-server's listening address, format: ip:port
	 * @param count {size_t} 连接池的最大连接数限制，如果此值为 0，则连接池
	 *  没有上限限制。
	 *  the max connections for each connection pool. there is
	 *  no connections limit of the pool when the count is 0.
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 *  the subscript of the connection pool in the connection cluster
	 */
	redis_client_pool(const char* addr, size_t count, size_t idx = 0);

	virtual ~redis_client_pool(void);

	/**
	 * 设置连接 redis 服务器的连接密码
	 * @param pass {const char*} 连接密码
	 * @return {redis_client_pool&}
	 */
	redis_client_pool& set_password(const char* pass);

protected:
	/**
	 * 基类纯虚函数: 调用此函数用来创建一个新的连接
	 * virtual function in class connect_pool to create a new connection
	 * @return {connect_client*}
	 */
	connect_client* create_connect();

private:
	char* pass_;
};

} // namespace acl
