#include "acl_stdafx.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_sqlite.hpp"
#include "acl_cpp/db/sqlite_pool.hpp"

namespace acl
{

sqlite_pool::sqlite_pool(const char* dbfile, int dblimit /* = 64 */)
: db_pool(dblimit)
{
	acl_assert(dbfile && *dbfile);
	dbfile_ = acl_mystrdup(dbfile);
}

sqlite_pool::~sqlite_pool()
{
	if (dbfile_)
		acl_myfree(dbfile_);
}

db_handle* sqlite_pool::create()
{
	return NEW db_sqlite(dbfile_);
}

} // namespace acl
