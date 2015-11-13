#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl
{

/**
 * HTTP 客户端请求连接池管理类
 */
class ACL_CPP_API http_request_manager : public acl::connect_manager
{
public:
	http_request_manager();
	virtual ~http_request_manager();

protected:
	/**
	 * 基类纯虚函数，用来创建连接池对象，该函数返回后由基类设置连接池的网络连接
	 * 及网络 IO 的超时时间
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {size_t} 连接池的大小限制，当该值为 0 时则没有限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);
};

} // namespace acl
