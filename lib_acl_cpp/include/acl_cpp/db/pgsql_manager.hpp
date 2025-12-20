#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/string.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class pgsql_conf;

class ACL_CPP_API pgsql_manager : public connect_manager {
public:
	pgsql_manager(time_t idle_ttl = 120);
	~pgsql_manager();

	/**
	 * Add a database instance method two
	 * @param conf {const pgsql_conf&}
	 * @return {pgsql_manager&}
	 */
	pgsql_manager& add(const pgsql_conf& conf);

protected:
	/**
	 * Implementation of base class connect_manager virtual function
	 * @param addr {const char*} Server listening address, format: ip:port
	 * @param count {size_t} Connection pool size limit, when this value is 0 there
	 * is no limit
	 * @param idx {size_t} Index position of this connection pool object in the
	 * collection (starting from 0)
	 * @return {connect_pool*} Returns the created connection pool object
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	time_t idle_ttl_;       // Idle expiration time for database connections
	std::map<string, pgsql_conf*> dbs_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)
