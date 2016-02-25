#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/db/db_mysql.hpp"
#include "acl_cpp/db/db_service_mysql.hpp"
#endif

namespace acl
{

db_service_mysql::db_service_mysql(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass, unsigned long dbflags /* = 0 */,
	bool auto_commit /* = true */, int conn_timeout /* = 60 */,
	int rw_timeout /* = 60 */, size_t dblimit /* = 100 */,
	int nthread /* = 2 */, bool win32_gui /* = false */)
: db_service(dblimit, nthread, win32_gui)
, dbaddr_(dbaddr)
, dbname_(dbname)
, dbuser_(dbuser)
, dbpass_(dbpass)
, dbflags_(dbflags)
, auto_commit_(auto_commit)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
{

}

db_service_mysql::~db_service_mysql(void)
{

}

db_handle* db_service_mysql::db_create(void)
{
	db_handle* db = NEW db_mysql(dbaddr_, dbname_, dbuser_, dbpass_,
		dbflags_, auto_commit_, conn_timeout_, rw_timeout_);
	return db;
}

}
