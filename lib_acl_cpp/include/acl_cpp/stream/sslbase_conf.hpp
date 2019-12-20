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
	 * @param server_side {bool} 是否为服务端模式，因为客户端模式与服务端
	 *  模式的握手方法不同，所以通过此参数来进行区分
	 * @param nblock {bool} 是否为非阻塞模式
	 * @return {sslbase_io*}
	 */
	virtual sslbase_io* open(bool server_side, bool nblock) = 0;
};

} // namespace acl
