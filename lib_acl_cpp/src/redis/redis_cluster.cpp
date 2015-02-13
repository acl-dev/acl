#include "acl_stdafx.hpp"
#include "acl_cpp/redis/redis_pool.hpp"
#include "acl_cpp/redis/redis_cluster.hpp"

namespace acl
{

redis_cluster::redis_cluster(int conn_timeout, int rw_timeout)
: conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{

}

redis_cluster::~redis_cluster()
{

}

connect_pool* redis_cluster::create_pool(const char* addr, int count, size_t idx)
{
	redis_pool* pool = NEW redis_pool(addr, count, idx);
	pool->set_timeout(conn_timeout_, rw_timeout_);

	return pool;
}

} // namespace acl
