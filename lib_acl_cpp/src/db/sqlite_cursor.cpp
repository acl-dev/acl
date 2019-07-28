#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/db/query.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/db/sqlite_cursor.hpp"
#endif

#if !defined(ACL_DB_DISABLE)

namespace acl
{

sqlite_cursor::sqlite_cursor(query& q)
: stmt_(NULL)
, free_callback(NULL)
, row_(NULL)
{
	sql_ = q.to_string();
	dbuf_ = NEW dbuf_guard;
}

sqlite_cursor::~sqlite_cursor(void)
{
	if (stmt_ && free_callback) {
		free_callback(stmt_);
	}
	delete row_;
}

void sqlite_cursor::clear(void)
{
	if (row_) {
		row_->clear();
	}
	dbuf_->dbuf_reset();
}

void sqlite_cursor::add_column_name(const char* name)
{
	names_.push_back(name);
}

void sqlite_cursor::create_row(void)
{
	row_ = NEW db_row(names_);
}

#define INT4_STR_SIZE	20  // 18446744073709551615

void sqlite_cursor::add_column_value(long long n)
{
	char* buf = (char*) dbuf_->dbuf_alloc(INT4_STR_SIZE + 1);
	safe_snprintf(buf, INT4_STR_SIZE + 1, "%lld", n);
	row_->push_back(buf, strlen(buf));
}

void sqlite_cursor::add_column_value(double n)
{
	char* buf = (char*) dbuf_->dbuf_alloc(INT4_STR_SIZE + 1);
	safe_snprintf(buf, INT4_STR_SIZE + 1, "%.4f", n);
	row_->push_back(buf, strlen(buf));
}

void sqlite_cursor::add_column_value(const char* s)
{
	row_->push_back(s, strlen(s));
}

} // namespace acl

#endif // !defined(ACL_DB_DISABLE)
