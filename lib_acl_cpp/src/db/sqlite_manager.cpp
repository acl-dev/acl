#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/db/sqlite_pool.hpp"
#include "acl_cpp/db/sqlite_manager.hpp"
#endif

namespace acl {

sqlite_manager::sqlite_manager(const char* charset /* = "utf-8" */)
{
	if (charset && *charset)
		charset_ = acl_mystrdup(charset);
	else
		charset_ = NULL;
	dblimit_ = 100;
}

sqlite_manager::~sqlite_manager()
{
	if (charset_)
		acl_myfree(charset_);
}

sqlite_manager& sqlite_manager::add(const char* dbfile, size_t dblimit)
{
	// 调用基类 connect_manager::set 方法添加
	set(dbfile, dblimit);
	return *this;
}

connect_pool* sqlite_manager::create_pool(const char*, size_t, size_t)
{
	sqlite_pool* dbpool = NEW sqlite_pool(dbfile_, dblimit_, charset_);
	return dbpool;
}

} // namespace acl
