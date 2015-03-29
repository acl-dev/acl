#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_pool.hpp"

namespace acl
{

redis_pool::redis_pool(const char* addr, int count, size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
, conn_timeout_(30)
, rw_timeout_(60)
{
}

redis_pool::~redis_pool()
{
}

redis_pool& redis_pool::set_timeout(int conn_timeout, int rw_timeout)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	return *this;
}

connect_client* redis_pool::create_connect()
{
	redis_client* conn = NEW redis_client(addr_, conn_timeout_,
		rw_timeout_);
	return conn;
}

} // namespace acl
