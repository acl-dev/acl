#pragma once
#include "../acl_cpp_define.hpp"
#include <string>

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API mysql_conf {
public:
	/**
	 * Constructor
	 * @param dbaddr {const char*} Database connection address, can be TCP socket or
	 *  UNIX domain socket. When it is TCP socket, address format: ip:port. When it is UNIX
	 *  domain socket, address format: /xxx/xxx/xxx.sock
	 * @param dbname {const char*} Database name
	 */
	mysql_conf(const char* dbaddr, const char* dbname);

	/**
	 * Copy constructor
	 * @param conf {const mysql_conf&} Internally creates new object and copies input object content
	 */
	mysql_conf(const mysql_conf& conf);

	~mysql_conf();

	/**
	 * Set user account when connecting to database
	 * @param dbuser {const char*} When it is a non-empty string, specifies user account
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dbuser(const char* dbuser);

	/**
	 * Set account password when connecting to database
	 * @param dbpass {const char*} When it is a non-empty string, specifies account password
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dbpass(const char* dbpass);

	/**
	 * Set maximum connection limit for database connection pool
	 * @param dblimit {size_t} Connection pool maximum connection limit, when 0 there is no limit
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dblimit(size_t dblimit);

	/**
	 * Set some special flag bits for mysql database
	 * @param dbflags {unsigned long}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_dbflags(unsigned long dbflags);

	/**
	 * Set whether to allow auto commit when modifying database content, default is auto commit
	 * @param on {bool}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_auto_commit(bool on);

	/**
	 * Set timeout for connecting to database
	 * @param timeout {int}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_conn_timeout(int timeout);

	/**
	 * Set timeout for reading database results
	 * @param timeout {int}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_rw_timeout(int timeout);

	/**
	 * Set character set for database connection
	 * @param charset {const char*}
	 * @return {mysql_conf&}
	 */
	mysql_conf& set_charset(const char* charset);

	const char* get_dbaddr() const {
		return dbaddr.c_str();
	}

	const char* get_dbname() const {
		return dbname.c_str();
	}

	const char* get_dbkey() const {
		return dbkey.c_str();
	}

	const char* get_dbuser() const {
		return dbuser.c_str();
	}

	const char* get_dbpass() const {
		return dbpass.c_str();
	}

	size_t get_dblimit() const {
		return dblimit;
	}

	unsigned long get_dbflags() const {
		return dbflags;
	}

	bool get_auto_commit() const {
		return auto_commit;
	}

	int get_conn_timeout() const {
		return conn_timeout;
	}

	int get_rw_timeout() const {
		return rw_timeout;
	}

	const char* get_charset() const {
		return charset.c_str();
	}

	//////////////////////////////////////////////////////////////////////

	std::string dbaddr;     // Database listening address
	std::string dbname;     // Database name
	std::string dbkey;      // dbname@dbaddr
	std::string dbuser;     // Database account
	std::string dbpass;     // Database account password
	std::string charset;    // Character set when connecting to database
	size_t dblimit;         // Database connection pool connection limit
	unsigned long dbflags;  // Flag bits when opening database
	bool  auto_commit;      // Whether to auto commit modified data
	int   conn_timeout;     // Timeout for connecting to database
	int   rw_timeout;       // Timeout when communicating with database

	std::string sslcrt;
	std::string sslkey;
	std::string sslca;
	std::string sslcapath;
	std::string sslcipher;
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

