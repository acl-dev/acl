#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_pool.hpp"
#endif

namespace acl
{

db_pool::db_pool(const char* dbaddr, size_t count, size_t idx /* = 0 */)
	: connect_pool(dbaddr, count, idx)
{
}

db_handle* db_pool::peek_open(const char* charset /* = NULL */)
{
	db_handle* conn = (db_handle*) peek();

	if (conn == NULL)
		return NULL;
	if (conn->dbopen(charset) == true)
		return conn;
	logger_error("open db failed");
	put(conn, false);
	return NULL;
}

//////////////////////////////////////////////////////////////////////////////

db_guard::~db_guard(void)
{
	if (conn_)
	{
		db_handle* db = (db_handle*) conn_;
		db->free_result();
		pool_.put(conn_, keep_);
		conn_ = NULL;
	}
}

} // namespace acl
