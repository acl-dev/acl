#pragma once
#include "../acl_cpp_define.hpp"
#include "connect_pool.hpp"

namespace acl
{

class string;
class connect_client;

class ACL_CPP_API tcp_pool : public connect_pool
{
public:
	tcp_pool(const char* addr, size_t count, size_t idx = 0);
	virtual ~tcp_pool(void);

	/**
	 * 向服务器发送指定长度的数据包，该方法会自动从连接池获取连接进行发送
	 * @param data {const void*} 要发送的数据包地址
	 * @param len {unsigned int} 数据长度
	 * @param out {string*} 当该对象非 NULL 时表明需要从服务器读取响应数据，
	 *  响应结果将被存放在该缓冲区中，如果该对象为 NULL，则表示无需读取
	 *  服务器的响应数据
	 * @return {bool} 发送是否成功
	 */
	bool send(const void* data, unsigned int len, string* out = NULL);

protected:
	// @override
	virtual connect_client* create_connect(void);
};

} // namespace acl
