#pragma once
#include "../acl_cpp_define.hpp"
#include "session.hpp"

#ifndef ACL_CLIENT_ONLY

namespace acl {

class memcache;

/**
 * Session class, this class uses memcached to store session data
 */
class ACL_CPP_API memcache_session : public session
{
public:
	/**
	 * Constructor
	 * @param cache_addr {const char*} memcached service address, format:
	 *  IP:PORT, cannot be empty
	 * @param prefix {const char*} Prefix for key values stored in memcached
	 * @param conn_timeout {int} Timeout for connecting to memcached (seconds)
	 * @param rw_timeout {int} IO timeout when communicating with memcached (seconds)
	 * @param ttl {time_t} Lifetime (seconds)
	 * @param sid {const char*} sid corresponding to session. When empty, internally
	 *  will automatically generate one. For other descriptions, please refer to base class session description
	 * @param encode_key {bool} Whether to encode key values stored in memcached
	 */
	memcache_session(const char* cache_addr, int conn_timeout = 180,
		int rw_timeout = 300, const char* prefix = NULL,
		time_t ttl = 0, const char* sid = NULL, bool encode_key = true);

	/**
	 * Constructor with memcached connection object as parameter
	 * @param cache {memcache*} Input memcached connection object
	 * @param auto_free {bool} When this parameter is true, then requires that
	 *  memcached_session object's destructor releases the passed cache object;
	 *  otherwise prohibits releasing cache object in memcached_session's destructor
	 * @param ttl {time_t} Lifetime (seconds)
	 * @param sid {const char*} sid corresponding to session. When empty, internally
	 *  will automatically generate one. For other descriptions, please refer to base class session description
	 */
	memcache_session(memcache* cache, bool auto_free = false,
		time_t ttl = 0, const char* sid = NULL);

	~memcache_session(void);

	// Base class pure virtual function, get data from memcached
	bool get_attrs(std::map<string, session_string>& attrs);

	// Base class pure virtual function, add or modify data in memcached
	bool set_attrs(const std::map<string, session_string>& attrs);

	// Base class pure virtual function, delete data from memcached
	bool remove();

protected:
	// Reset cache time of session on memcached
	bool set_timeout(time_t ttl);

private:
	memcache* cache_;
	bool auto_free_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY

