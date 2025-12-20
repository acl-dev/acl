#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_manager.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

/**
 * memcache client request connection pool management class
 */
class ACL_CPP_API memcache_manager : public connect_manager
{
public:
	memcache_manager();
	virtual ~memcache_manager();

protected:
	/**
	 * Base class pure virtual function, used to create connection pool objects
	 * @param addr {const char*} Server listening address, format: ip:port
	 * @param count {size_t} Connection pool size limit, when this value is 0 there
	 * is no limit
	 * @param idx {size_t} Index position of this connection pool object in the
	 * collection (starting from 0)
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

