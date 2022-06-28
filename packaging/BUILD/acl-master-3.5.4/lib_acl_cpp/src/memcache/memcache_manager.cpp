#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/memcache/memcache_pool.hpp"
#include "acl_cpp/memcache/memcache_manager.hpp"
#endif

#ifndef ACL_CLIENT_ONLY

namespace acl
{

memcache_manager::memcache_manager(void)
{
}

memcache_manager::~memcache_manager(void)
{
}

connect_pool* memcache_manager::create_pool(const char* addr,
	size_t count, size_t idx)
{
	memcache_pool* conns = NEW memcache_pool(addr, count, idx);

	return conns;
}

} // namespace acl

#endif // ACL_CLIENT_ONLY
