#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_handle.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

//typedef struct st_mysql MYSQL;

namespace acl {

class mysql_conf;

class ACL_CPP_API db_mysql : public db_handle {
public:
	/**
	 * Constructor method one
	 * @param dbaddr {const char*} Database listening address, can be TCP socket or
	 * domain socket on UNIX
	 *  platform, format: 127.0.0.1:3306, or /tmp/mysql.sock
	 * @param dbname {const char*} Database name, non-NULL
	 * @param dbuser {const char*} Username when connecting to database
	 * @param dbpass {const char*} User password when connecting to database
	 * @param dbflags {unsigned long} Flag bits when connecting to MYSQL
	 * @param auto_commit {bool} Whether to auto commit transaction when modifying
	 * database
	 * @param conn_timeout {int} Timeout for connecting to database (seconds)
	 * @param rw_timeout {int} Timeout when performing database operations
	 * (seconds)
	 * @param charset {const char*} Local character set when connecting to database
	 * (gbk, utf8, ...)
	 */
	db_mysql(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags = 0, bool auto_commit = true,
		int conn_timeout = 60, int rw_timeout = 60,
		const char* charset = "utf8");

	/**
	 * Constructor method two: Construct using parameter configuration class object
	 * @param conf {const mysql_conf&} MySQL database connection configuration
	 * class object
	 */
	db_mysql(const mysql_conf& conf);
	~db_mysql(void);

	/**
	 * Get MySQL client library version number
	 * @return {unsigned long}
	 */
	unsigned long mysql_libversion(void) const;

	/**
	 * Get MySQL client library information
	 * @return {const char*}
	 */
	const char* mysql_client_info(void) const;

	/**
	 * Directly get MySQL connection handle. If returns NULL, it indicates MySQL
	 * has not been opened yet
	 * or MySQL connection was automatically closed internally when error occurred
	 * @return {void*} Type is the same as MYSQL*
	 */
	void* get_conn(void) const {
		return conn_;
	}

	/**
	 * When dynamically loading libmysqlclient.so / libmysqlclient.dll, this
	 * static function can be called to explicitly dynamically load MySQL client
	 * library. If loading fails, internally will automatically generate assertion
	 * to avoid runtime errors. Can also not call this function, letting db_mysql
	 * class objects internally implicitly load MySQL dynamic library when used
	 * @return {bool} Whether loading MySQL dynamic library was successful
	 */
	static bool load(void);

	/********************************************************************/
	/*         The following are virtual interfaces of the base class db_handle */
	/********************************************************************/

	/**
	 * @override
	 */
	const char* dbtype(void) const;

	/**
	 * @override
	 */
	int get_errno(void) const;

	/**
	 * @override
	 */
	const char* get_error(void) const;

	/**
	 * @override
	 */
	bool dbopen(const char* charset = NULL);

	/**
	 * @override
	 */
	bool is_opened(void) const;

	/**
	 * @override
	 */
	bool close(void);

	/**
	 * @override
	 */
	bool tbl_exists(const char* tbl_name);

	/**
	 * @override
	 */
	bool sql_select(const char* sql, db_rows* result = NULL);

	/**
	 * @override
	 */
	bool sql_update(const char* sql);

	/**
	 * @override
	 */
	int affect_count(void) const;

	/**
	 * @override
	 * Virtual function of base class db_handle, used to indicate start of
	 * transaction. Note that to use transaction mode, need to pass parameter
	 * auto_commit as false in db_mysql constructor
	 */
	bool begin_transaction(void);

	/**
	 * @override
	 */
	bool commit(void);

	/**
	 * @override
	 */
	bool rollback(void);

private:
	std::string dbaddr_;  // Database listening address
	std::string dbname_;  // Database name
	std::string dbuser_;  // Database account
	std::string dbpass_;  // Database account password
	string charset_; // Character set used when connecting to database

	std::string sslcrt_;
	std::string sslkey_;
	std::string sslca_;
	std::string sslcapath_;
	std::string sslcipher_;

	unsigned long dbflags_;
	int    conn_timeout_;
	int    rw_timeout_;
	bool   auto_commit_;
	void*  conn_;	// MYSQL object pointer

	bool sane_mysql_query(const char* sql);
	void sane_mysql_init(const mysql_conf& conf);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

