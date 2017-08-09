/**
 * Copyright (C) 2017-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 08 Aug 2017 02:11:05 PM CST
 */

#pragma once
#include "../acl_cpp_define.hpp"

struct iovec;

namespace acl
{

class socket_stream;

/**
 * tcp ipc 通信发送类，内部自动组包
 */
class ACL_CPP_API tcp_sender
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
