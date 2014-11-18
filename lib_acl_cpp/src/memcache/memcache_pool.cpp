#include "acl_stdafx.hpp"
#include "acl_cpp/memcache/memcache.hpp"
#include "acl_cpp/memcache/memcache_pool.hpp"

namespace acl
{

memcache_pool::memcache_pool(const char* addr, int count,
	size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
{
	conn_timeout_ = 30;
	rw_timeout_ = 60;
}

memcache_pool::~memcache_pool()
{

}

memcache_pool& memcache_pool::set_timeout(int conn_timeout /* = 30 */,
	int rw_timeout /* = 60 */)
{
	conn_timeout_ = conn_timeout;
	rw_timeout_ = rw_timeout;
	return *this;
}

connect_client* memcache_pool::create_connect()
{
	memcache* conn = NEW memcache(addr_, conn_timeout_,
		rw_timeout_);
	return conn;
}

} // namespace acl
