#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../db/db_service.hpp"

#if !defined(ACL_DB_DISABLE)

namespace acl {

class ACL_CPP_API db_service_sqlite : public db_service
{
public:
	/**
	 * Constructor when it is sqlite database.
	 * @param dbname {const char*} Database name.
	 * @param dbfile {const char*} Database file name (useful for some embedded databases).
	 * @param dblimit {size_t} Database connection pool count limit.
	 * @param nthread {int} Maximum thread count of child thread pool.
	 * @param win32_gui {bool} Whether it is window class message. If yes, internal
	 *  communication mode is automatically set to _WIN32-based message. Otherwise, still uses generic socket
	 *  communication method.
	 */
	db_service_sqlite(const char* dbname, const char* dbfile,
		size_t dblimit = 100, int nthread = 2, bool win32_gui = false);
	~db_service_sqlite();

private:
	// Database name.
	string dbname_;
	// sqlite database file name.
	string dbfile_;

	// Base class pure virtual function.
	virtual db_handle* db_create(void);
};

}

#endif // !defined(ACL_DB_DISABLE)
