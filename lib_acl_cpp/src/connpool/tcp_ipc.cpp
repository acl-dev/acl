/**
 * Copyright (C) 2017-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Tue 08 Aug 2017 03:10:44 PM CST
 */

#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/connpool/tcp_manager.hpp"
#include "acl_cpp/connpool/tcp_pool.hpp"
#include "acl_cpp/connpool/tcp_client.hpp"
#include "acl_cpp/connpool/tcp_ipc.hpp"
#endif

namespace acl
{

tcp_ipc::tcp_ipc(void)
: max_(0)
, ttl_(60)
, conn_timeout_(10)
, rw_timeout_(10)
{
	manager_ = new tcp_manager;
}

tcp_ipc::~tcp_ipc(void)
{
	delete manager_;
}

tcp_ipc& tcp_ipc::set_limit(int max)
{
	max_ = max;
	return *this;
}

tcp_ipc& tcp_ipc::set_idle(int ttl)
{
	ttl_ = ttl;
	return *this;
}

tcp_ipc& tcp_ipc::set_conn_timeout(int timeout)
{
	conn_timeout_ = timeout;
	return *this;
}

tcp_ipc& tcp_ipc::set_rw_timeout(int timeout)
{
	rw_timeout_ = timeout;
	return *this;
}

bool tcp_ipc::send(const char* addr, const void* data, unsigned int len,
	string* out /* = NULL */)
{
	tcp_pool* pool = (tcp_pool*) manager_->peek(addr);
	if (pool == NULL)
		pool = &(tcp_pool&) manager_->set(
			addr, max_, conn_timeout_, rw_timeout_);
	return send(*pool, data, len, out);
}

bool tcp_ipc::send(tcp_pool& pool, const void* data, unsigned int len,
	string* out)
{
	tcp_client* conn = (tcp_client*) pool.peek();
	if (conn == NULL)
	{
		logger_error("no connection available, addr=%s",
			pool.get_addr());
		return false;
	}

	if (conn->send(data, len, out) == false)
	{
		pool.put(conn, false);
		return false;
	}

	pool.put(conn);
	return true;
}

} // namespace acl
