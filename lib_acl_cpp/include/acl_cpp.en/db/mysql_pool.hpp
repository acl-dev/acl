#pragma once
#include "../acl_cpp_define.hpp"
#include "../db/db_pool.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class db_handle;
class mysql_conf;

class ACL_CPP_API mysql_pool : public db_pool {
public:
	/**
	 * Constructor when using mysql database
	 * @param dbaddr {const char*} MySQL server address, format: IP:PORT.
	 *  On UNIX platform, can be UNIX domain socket
	 * @param dbname {const char*} Database name
	 * @param dbuser {const char*} Database user
	 * @param dbpass {const char*} Database user password
	 * @param dblimit {int} Maximum connection limit for database connection pool
	 * @param dbflags {unsigned long} MySQL flag bits
	 * @param auto_commit {bool} Whether to auto commit
	 * @param conn_timeout {int} Database connection timeout (seconds)
	 * @param rw_timeout {int} IO timeout when communicating with database
	 * (seconds)
	 * @param charset {const char*} Character set for connecting to database (utf8,
	 * gbk, ...)
	 */
	mysql_pool(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		int dblimit = 64, unsigned long dbflags = 0,
		bool auto_commit = true, int conn_timeout = 60,
		int rw_timeout = 60, const char* charset = "utf8");

	/**
	 * Constructor
	 * @param conf {const mysql_conf&} MySQL database connection configuration
	 * object
	 */
	mysql_pool(const mysql_conf& conf);
	~mysql_pool();

protected:
	// Base class connect_pool pure virtual function: create database connection
	// handle
	connect_client* create_connect();

private:
	mysql_conf* conf_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

