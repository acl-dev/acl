#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

struct iovec;

namespace acl
{

class socket_stream;

/**
 * tcp ipc 通信发送类，内部自动组包
 */
class ACL_CPP_API tcp_sender : public noncopyable
{
public:
	tcp_sender(socket_stream& conn);
	~tcp_sender(void);

	/**
	 * 发送方法
	 * @param data {const void*} 要发送的数据包地址
	 * @param len {unsigned int} 数据包长度
	 * @return {bool} 发送是否成功
	 */
	bool send(const void* data, unsigned int len);

	/**
	 * 获得连接流对象
	 * @return {acl::socket_stream&}
	 */
	acl::socket_stream& get_conn(void) const
	{
		return *conn_;
	}

private:
	acl::socket_stream* conn_;
	struct iovec* v2_;
};

} // namespace acl
