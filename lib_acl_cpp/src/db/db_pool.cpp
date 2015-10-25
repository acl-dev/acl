#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_pool.hpp"

namespace acl
{

db_pool::db_pool(const char* dbaddr, size_t count, size_t idx /* = 0 */)
	: connect_pool(dbaddr, count, idx)
{
}

db_handle* db_pool::peek_open()
{
	db_handle* conn = (db_handle*) peek();

	if (conn == NULL)
		return NULL;
	if (conn->dbopen() == true)
		return conn;
	logger_error("open db failed");
	put(conn, false);
	return NULL;
}

} // namespace acl
