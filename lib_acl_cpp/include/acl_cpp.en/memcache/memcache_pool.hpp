#pragma once
#include "../acl_cpp_define.hpp"
#include "../connpool/connect_pool.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl
{

/**
 * memcache client connection pool class. This class's parent class is connect_pool. This class only needs to implement virtual function
 * create_connect in parent class to have connection pool parent class connect_pool's functionality. Additionally, connection objects created by this class
 * are memcache objects, so when calling connect_pool::peek, returned
 * is http_request class. Callers need to cast peek returned class object to memcache
 * class object, then can use all functions of memcache class. memcache class is
 * a subclass of connect_client.
 */
class ACL_CPP_API memcache_pool : public connect_pool
{
public:
	/**
	 * Constructor.
	 * @param addr {const char*} Server address, format: ip:port.
	 * @param count {size_t} Connection pool's maximum connection count limit. When this value is 0, there is no limit.
	 * @param idx {size_t} This connection pool object's subscript position in collection (starting from 0).
	 */
	memcache_pool(const char* addr, size_t count, size_t idx = 0);
	~memcache_pool();

protected:
	// Base class pure virtual function.
	virtual connect_client* create_connect();
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
