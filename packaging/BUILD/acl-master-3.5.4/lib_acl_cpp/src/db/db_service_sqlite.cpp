#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/db/db_sqlite.hpp"
#include "acl_cpp/db/db_service_sqlite.hpp"
#endif

#if !defined(ACL_DB_DISABLE)

namespace acl
{

db_service_sqlite::db_service_sqlite(const char* dbname, const char* dbfile,
	size_t dblimit /* = 100 */, int nthread /* = 2 */,
	bool win32_gui /* = false */)
: db_service(dblimit, nthread, win32_gui)
, dbname_(dbname)
, dbfile_(dbfile)
{

}

db_service_sqlite::~db_service_sqlite(void)
{
}

db_handle* db_service_sqlite::db_create(void)
{
	db_handle* db = NEW db_sqlite(dbfile_.c_str());
	return db;
}

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
