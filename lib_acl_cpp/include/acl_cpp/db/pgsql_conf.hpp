#pragma once
#include "../acl_cpp_define.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API pgsql_conf {
public:
	/**
	 * Constructor
	 * @param dbaddr {const char*} Server address, address format: ip:port, or
	 * unix_domain_path. When it is a unix domain socket, it should be the
	 * directory where the unix domain socket file is located and does not
	 * include the filename. Assuming postgresql is listening on unix domain
	 * socket file: /tmp/.s.PGSQL.5432, then dbaddr address should be set to /tmp
	 * Note: Note the difference from mysql when connecting to unix domain socket.
	 * mysql's domain socket is the full path
	 * @param dbname {const char*} Database name
	 */
	pgsql_conf(const char* dbaddr, const char* dbname);

	/**
	 * Copy constructor
	 * @param conf {const pgsql_conf&} Internally will create a new configuration
	 * object and copy the content items
	 *  from this parameter
	 */
	pgsql_conf(const pgsql_conf& conf);

	~pgsql_conf(void);

	/**
	 * Set user account when connecting to database. When this method is not
	 * called, no account is needed
	 * @param dbuser {const char*} User account, only valid when it is a non-empty
	 * string
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_dbuser(const char* dbuser);

	/**
	 * Set account password when connecting to database. When this method is not
	 * called, no password is set
	 * @param dbpass {const char*} Account password, only valid when it is a
	 * non-empty string
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_dbpass(const char* dbpass);

	/**
	 * Set maximum connection limit for database connection pool
	 * @param dblimit {size_t} Connection pool maximum connection limit, when 0
	 * there is no limit
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_dblimit(size_t dblimit);

	/**
	 * Set timeout for connecting to database
	 * @param timeout {int}
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_conn_timeout(int timeout);

	/**
	 * Set timeout for reading database results
	 * @param timeout {int}
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_rw_timeout(int timeout);

	/**
	 * Set character set for database connection
	 * @param charset {const char*}
	 * @return {pgsql_conf&}
	 */
	pgsql_conf& set_charset(const char* charset);

	const char* get_dbaddr() const {
		return dbaddr_;
	}

	const char* get_dbname() const {
		return dbname_;
	}

	const char* get_dbkey() const {
		return dbkey_;
	}

	const char* get_dbuser() const {
		return dbuser_;
	}

	const char* get_dbpass() const {
		return dbpass_;
	}

	size_t get_dblimit() const {
		return dblimit_;
	}

	int get_conn_timeout() const {
		return conn_timeout_;
	}

	int get_rw_timeout() const {
		return rw_timeout_;
	}

	const char* get_charset() const {
		return charset_;
	}

private:
	char* dbaddr_;
	char* dbname_;
	char* dbkey_;
	char* dbuser_;
	char* dbpass_;
	char* charset_;
	size_t dblimit_;
	int   conn_timeout_;
	int   rw_timeout_;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

