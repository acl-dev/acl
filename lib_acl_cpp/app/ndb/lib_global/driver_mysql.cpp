#include "stdafx.h"

#include "db_driver.h"
#include "driver_mysql.h"

driver_mysql::driver_mysql(const char* dbaddr, const char* dbname,
	const char* dbuser, const char* dbpass,
	int dbpool_limit /* = 50 */, int dbpool_ping /* = 30 */,
	int dbpool_timeout /* = 60 */)
{
	ACL_DB_INFO db_info;

	memset(&db_info, 0, sizeof(ACL_DB_INFO));

	ACL_SAFE_STRNCPY(db_info.db_addr, dbaddr, sizeof(db_info.db_addr));
	ACL_SAFE_STRNCPY(db_info.db_name, dbname, sizeof(db_info.db_name));
	ACL_SAFE_STRNCPY(db_info.db_user, dbuser, sizeof(db_info.db_user));
	ACL_SAFE_STRNCPY(db_info.db_pass, dbpass, sizeof(db_info.db_pass));

	db_info.db_max = dbpool_limit;
	db_info.ping_inter = dbpool_ping;
	db_info.timeout_inter = dbpool_timeout;
	db_info.auto_commit = 1;

	dbpool_ = acl_dbpool_create("mysql", &db_info);

	if (dbpool_ == NULL)
		logger_fatal("acl_dbpool_create(mysql) error");
}

driver_mysql::~driver_mysql(void)
{
	acl_dbpool_destroy(dbpool_);
}

bool driver_mysql::create(const char* dbname, const char* tbl,
	const char* idx, bool idx_unique /* = false */,
	const char* user /* = NULL */, const char* pass /* = NULL */)
{
	return true;
}

bool driver_mysql::open(const char* dbname, const char*tbl, const char* idx,
	const char* user /* = NULL */, const char* pass /* = NULL */)
{
	return true;
}

bool driver_mysql::set(const char* idx_value, const void* data, size_t dlen)
{
	return true;
}

db_result* driver_mysql::get(const char* idx_value)
{
	return NULL;
}

bool driver_mysql::del(const char* idx_value)
{
	return true;
}

size_t driver_mysql::affect_count() const
{
	return 0;
}

db_error_t driver_mysql::last_error() const
{
	return DB_OK;
}
