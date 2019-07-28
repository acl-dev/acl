#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/db/db_handle.hpp"
#include "acl_cpp/db/db_pool.hpp"
#endif

#if !defined(ACL_DB_DISABLE)

namespace acl
{

db_pool::db_pool(const char* dbaddr, size_t count, size_t idx /* = 0 */)
: connect_pool(dbaddr, count, idx)
{
}

db_handle* db_pool::peek_open(void)
{
	db_handle* conn = (db_handle*) peek();
	if (conn == NULL) {
		logger_error("peek NULL");
	}
	return conn;
}

//////////////////////////////////////////////////////////////////////////////

db_guard::~db_guard(void)
{
	if (conn_) {
		db_handle* db = (db_handle*) conn_;
		db->free_result();
		pool_.put(conn_, keep_);
		conn_ = NULL;
	}
}

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
