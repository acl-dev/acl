#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include "../stdlib/string.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class mysql_conf;

class ACL_CPP_API mysql_manager : public connect_manager
{
public:
	mysql_manager(time_t idle_ttl = 120);
	~mysql_manager();

	/**
	 * Add a database instance method one
	 * @param dbaddr {const char*} MySQL server address, format: IP:PORT.
	 *  On UNIX platform, can be UNIX domain socket
	 * @param dbname {const char*} Database name
	 * @param dbuser {const char*} Database user
	 * @param dbpass {const char*} Database user password
	 * @param dblimit {size_t} Maximum connection limit for database connection pool
	 * @param dbflags {unsigned long} MySQL flag bits
	 * @param auto_commit {bool} Whether to auto commit
	 * @param conn_timeout {int} Database connection timeout (seconds)
	 * @param rw_timeout {int} IO timeout when communicating with database (seconds)
	 * @param charset {const char*} Character set when connecting to database
	 * @return {mysql_manager&}
	 */
	mysql_manager& add(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		size_t dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60, const char* charset = "utf8");

	/**
	 * Add a database instance method two
	 * @param conf {const mysql_conf&}
	 * @return {mysql_manager&}
	 */
	mysql_manager& add(const mysql_conf& conf);

protected:
	/**
	 * Implementation of base class connect_manager virtual function
	 * @param addr {const char*} Server listening address, format: ip:port
	 * @param count {size_t} Connection pool size limit, when this value is 0 there is no limit
	 * @param idx {size_t} Index position of this connection pool object in the collection (starting from 0)
	 * @return {connect_pool*} Returns the created connection pool object
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	time_t idle_ttl_;       // Idle expiration time for database connections
	std::map<string, mysql_conf*> dbs_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

