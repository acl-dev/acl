#pragma once
#include "../acl_cpp_define.hpp"
#include "connect_client.hpp"

namespace acl
{

class socket_stream;
class tcp_sender;
class tcp_reader;
class string;

class ACL_CPP_API tcp_client : public connect_client
{
public:
	tcp_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
	virtual ~tcp_client(void);

	/**
	 * 向服务器发送指定长度的数据包
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
	virtual bool open(void);

private:
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;

	socket_stream* conn_;
	tcp_sender*    sender_;
	tcp_reader*    reader_;

	bool try_open(bool* reuse_conn);
};

} // namespace acl
