#include "acl_stdafx.hpp"
#include "acl_cpp/memcache/memcache.hpp"
#include "acl_cpp/memcache/memcache_pool.hpp"

namespace acl
{

memcache_pool::memcache_pool(const char* addr, size_t count,
	size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
{
}

memcache_pool::~memcache_pool()
{

}

connect_client* memcache_pool::create_connect()
{
	return NEW memcache(addr_);
}

} // namespace acl
