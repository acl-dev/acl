#include "stdafx.h"
#include "driver_hsocket.h"

driver_hsocket::driver_hsocket(void)
{
}

driver_hsocket::~driver_hsocket(void)
{
}

bool driver_hsocket::create(const char* dbname, const char* tbl,
	const char* idx, bool idx_unique /* = false */,
	const char* user /* = NULL */, const char* pass /* = NULL */)
{
	return true;
}

bool driver_hsocket::open(const char* dbname, const char*tbl, const char* idx,
	const char* user /* = NULL */, const char* pass /* = NULL */)
{
	return true;
}

bool driver_hsocket::set(const char* idx_value, const void* data, size_t dlen)
{
	return true;
}

db_result* driver_hsocket::get(const char* idx_value)
{
	return 0;
}

bool driver_hsocket::del(const char* idx_value)
{
	return true;
}

size_t driver_hsocket::affect_count() const
{
	return 0;
}

db_error_t driver_hsocket::last_error() const
{
	return DB_OK;
}
