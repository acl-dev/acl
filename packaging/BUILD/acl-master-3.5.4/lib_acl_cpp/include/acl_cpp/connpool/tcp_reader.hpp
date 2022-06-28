#pragma once
#include "../stdlib/noncopyable.hpp"

namespace acl
{

class socket_stream;
class string;

/**
 * tcp ipc 通信接收类，内部会自动读取完事的数据包
 */
class ACL_CPP_API tcp_reader : public noncopyable
{
public:
	tcp_reader(socket_stream& conn);
	~tcp_reader(void) {}

	/**
	 * 从对端读取数据，每次只读一个数据包
	 * @param out {string&} 存储数据包，内部采用追加方式往 out 添加数据
	 */
	bool read(string& out);

	/**
	 * 获得连接流对象
	 * @return {acl::socket_stream&}
	 */
	acl::socket_stream& get_conn(void) const
	{
		return *conn_;
	}

private:
	socket_stream* conn_;
};

} // namespace acl
