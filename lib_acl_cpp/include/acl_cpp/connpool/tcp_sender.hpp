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

class ACL_CPP_API tcp_sender
{
public:
	tcp_sender(socket_stream& conn);
	~tcp_sender(void);

	bool send(const void* data, unsigned int len);

private:
	acl::socket_stream* conn_;
	struct iovec* v2_;
};

} // namespace acl
