#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_service.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API db_service_mysql : public db_service
{
	/**
	 * Constructor when using mysql database
	 * @param dbaddr {const char*} MySQL server address
	 * @param dbname {const char*} Database name
	 * @param dbuser {const char*} Database username
	 * @param dbpass {const char*} Database user password
	 * @param dbflags {unsigned long} Database connection flag bits
	 * @param auto_commit {bool} Whether to auto commit when modifying data
	 * @param conn_timeout {int} Database connection timeout
	 * @param rw_timeout {int} IO read/write timeout when operating database
	 * @param dblimit {size_t} Connection pool count limit for database
	 * @param nthread {int} Maximum thread count for child thread pool
	 * @param win32_gui {bool} Whether it is window class message. If yes, then internally
	 *  communication mode is automatically set to _WIN32 message based, otherwise still uses common socket
	 *  communication method
	 */
	db_service_mysql(const char* dbaddr, const char* dbname,
		const char* dbuser, const char* dbpass,
		unsigned long dbflags = 0, bool auto_commit = true,
		int conn_timeout = 60, int rw_timeout = 60,
		size_t dblimit = 100, int nthread = 2, bool win32_gui = false);

	~db_service_mysql(void);

private:
	// Database server address
	string dbaddr_;
	// Database name
	string dbname_;
	// Database username
	string dbuser_;
	// Database user password
	string dbpass_;
	// Database connection flag bits
	unsigned long dbflags_;
	// Whether to auto commit when modifying data
	bool auto_commit_;
	// Database connection timeout
	int conn_timeout_;
	// Read/write timeout when operating database
	int rw_timeout_;

	// Base class pure virtual function
	virtual db_handle* db_create(void);
};

}

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_DB_DISABLE)

