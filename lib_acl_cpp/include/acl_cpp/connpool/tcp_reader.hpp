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

class tcp_reader
{
public:
	tcp_reader(socket_stream& conn);
	~tcp_reader(void) {}

	bool read(string& out);

private:
	socket_stream* conn_;
};

} // namespace acl
