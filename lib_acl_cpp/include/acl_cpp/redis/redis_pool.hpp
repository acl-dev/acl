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
class ACL_CPP_API redis_pool : public connect_pool
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} 服务端地址，格式：ip:port
	 * @param count {int} 连接池的最大连接数限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 */
	redis_pool(const char* addr, int count, size_t idx = 0);

	virtual ~redis_pool();

	/**
	 * 设置网络连接超时时间及网络 IO 读写超时时间(秒)
	 * @param conn_timeout {int} 连接超时时间
	 * @param rw_timeout {int} 网络 IO 读写超时时间(秒)
	 * @return {redis_pool&}
	 */
	redis_pool& set_timeout(int conn_timeout, int rw_timeout);

protected:
	/**
	 * 基类纯虚函数: 调用此函数用来创建一个新的连接
	 * @return {connect_client*}
	 */
	virtual connect_client* create_connect();

private:
	int   conn_timeout_;
	int   rw_timeout_;
};

} // namespace acl
