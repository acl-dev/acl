#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/memcache/memcache.hpp"
#include "acl_cpp/memcache/memcache_pool.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

memcache_pool::memcache_pool(const char* addr, size_t count,
	size_t idx /* = 0 */)
: connect_pool(addr, count, idx)
{
}

memcache_pool::~memcache_pool(void)
{
}

connect_client* memcache_pool::create_connect(void)
{
	return NEW memcache(addr_);
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
