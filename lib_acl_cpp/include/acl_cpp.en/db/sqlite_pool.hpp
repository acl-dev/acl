#pragma once
#include "../acl_cpp_define.hpp"
#include "../db/db_pool.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class db_handle;

class ACL_CPP_API sqlite_pool : public db_pool
{
public:
	/**
	 * Constructor
	 * @param dbfile {const char*} SQLite database data file
	 * @param dblimit {size_t} Maximum connection limit for database connection pool
	 * @param charset {const char*} Character set of the data filename
	 */
	sqlite_pool(const char* dbfile, size_t dblimit = 64,
		const char* charset = "utf-8");
	~sqlite_pool();

protected:
	// Base class connect_pool pure virtual function: create database connection handle
	connect_client* create_connect();

private:
	// SQLite data filename
	char* dbfile_;
	// Character set of SQLite data filename
	char* charset_;
};

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
