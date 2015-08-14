#include "acl_stdafx.hpp"
#include "acl_cpp/connpool/connect_client.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_sqlite.hpp"
#include "acl_cpp/db/sqlite_pool.hpp"

namespace acl
{

sqlite_pool::sqlite_pool(const char* dbfile, size_t dblimit /* = 64 */)
: db_pool(dbfile, dblimit)
{
	acl_assert(dbfile && *dbfile);
	dbfile_ = acl_mystrdup(dbfile);
}

sqlite_pool::~sqlite_pool()
{
	acl_myfree(dbfile_);
}

connect_client* sqlite_pool::create_connect()
{
	return NEW db_sqlite(dbfile_);
}

} // namespace acl
