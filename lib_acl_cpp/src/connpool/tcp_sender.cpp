/**
 * Copyright (C) 2017-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 08 Aug 2017 02:13:31 PM CST
 */

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/connpool/tcp_sender.hpp"
#endif

namespace acl
{

tcp_sender::tcp_sender(socket_stream& conn)
: conn_(&conn)
{
}

bool tcp_sender::send(const void* data, unsigned int len)
{
	unsigned int n = htonl(len);

	v_[0].iov_base = &n;
	v_[0].iov_len  = sizeof(n);
	v_[1].iov_base = (void*) data;
	v_[1].iov_len  = len;

	return conn_->writev(v_, 2) > 0;
}

} // namespace acl
