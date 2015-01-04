#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"

namespace acl
{

dbuf_pool::dbuf_pool()
{
	pool_ = acl_dbuf_pool_create(0);
}

dbuf_pool::~dbuf_pool()
{
	acl_dbuf_pool_destroy(pool_);
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
