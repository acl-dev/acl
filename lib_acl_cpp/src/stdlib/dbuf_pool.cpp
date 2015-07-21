#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"

namespace acl
{

dbuf_pool::dbuf_pool()
{
}

dbuf_pool::~dbuf_pool()
{
}

void *dbuf_pool::operator new(size_t size)
{
	ACL_DBUF_POOL* pool = acl_dbuf_pool_create(8192);
	dbuf_pool* dbuf = (dbuf_pool*) acl_dbuf_pool_alloc(pool, size);
	dbuf->pool_ = pool;
	dbuf->mysize_ = size;
	return dbuf;
}

void dbuf_pool::operator delete(void* ptr)
{
	dbuf_pool* dbuf = (dbuf_pool*) ptr;
	acl_dbuf_pool_destroy(dbuf->pool_);
}

void dbuf_pool::dbuf_reset()
{
	acl_dbuf_pool_reset(pool_, mysize_);
}

void* dbuf_pool::dbuf_alloc(size_t len)
{
	return acl_dbuf_pool_alloc(pool_, len);
}

void* dbuf_pool::dbuf_calloc(size_t len)
{
	return acl_dbuf_pool_calloc(pool_, len);
}

char* dbuf_pool::dbuf_strdup(const char* s)
{
	return acl_dbuf_pool_strdup(pool_, s);
}

void* dbuf_pool::dbuf_memdup(const void* s, size_t len)
{
	return acl_dbuf_pool_memdup(pool_, s, len);
}

} // namespace acl
