#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_pool.hpp"
#include "acl_cpp/redis/redis_manager.hpp"

namespace acl
{

redis_manager::redis_manager(int conn_timeout, int rw_timeout)
: conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{

}

redis_manager::~redis_manager()
{

}

connect_pool* redis_manager::create_pool(const char* addr, int count, size_t idx)
{
	redis_pool* pool = NEW redis_pool(addr, count, idx);
	pool->set_timeout(conn_timeout_, rw_timeout_);

	return pool;
}

} // namespace acl
