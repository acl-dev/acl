/**
 * Copyright (C) 2017-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 08 Aug 2017 01:57:39 PM CST
 */

#pragma once

namespace acl
{

class socket_stream;
class string;

/**
 * tcp ipc 通信接收类，内部会自动读取完事的数据包
 */
class ACL_CPP_API tcp_reader
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
