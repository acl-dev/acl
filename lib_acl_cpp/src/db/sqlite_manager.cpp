#include "acl_stdafx.hpp"
#include "acl_cpp/db/sqlite_pool.hpp"
#include "acl_cpp/db/sqlite_manager.hpp"

namespace acl {

sqlite_manager::sqlite_manager()
{
}

sqlite_manager::~sqlite_manager()
{
}

sqlite_manager& sqlite_manager::add(const char* dbfile, int dblimit)
{
	// 调用基类 connect_manager::set 方法添加
	set(dbfile, dblimit);
	return *this;
}

connect_pool* sqlite_manager::create_pool(const char*, int, size_t)
{
	sqlite_pool* dbpool = NEW sqlite_pool(dbfile_, dblimit_);
	return dbpool;
}

} // namespace acl
