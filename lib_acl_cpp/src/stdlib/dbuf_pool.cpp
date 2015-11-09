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

void dbuf_pool::destroy()
{
	delete this;
}

void *dbuf_pool::operator new(size_t size, size_t nblock /* = 2 */)
{
	if (nblock == 0)
		nblock = 2;
	ACL_DBUF_POOL* pool = acl_dbuf_pool_create(4096 * nblock);
	dbuf_pool* dbuf     = (dbuf_pool*) acl_dbuf_pool_alloc(pool, size);
	dbuf->pool_         = pool;
	dbuf->mysize_       = size;

	return dbuf;
}

#if defined(_WIN32) || defined(_WIN64)
void dbuf_pool::operator delete(void* ptr, size_t)
#else
void dbuf_pool::operator delete(void* ptr)
#endif
{
	dbuf_pool* dbuf = (dbuf_pool*) ptr;
	acl_dbuf_pool_destroy(dbuf->pool_);
}

bool dbuf_pool::dbuf_reset(size_t reserve /* = 0 */)
{
	return acl_dbuf_pool_reset(pool_, mysize_ + reserve) == 0
		? true : false;
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

char* dbuf_pool::dbuf_strndup(const char* s, size_t len)
{
	return acl_dbuf_pool_strndup(pool_, s, len);
}

void* dbuf_pool::dbuf_memdup(const void* addr, size_t len)
{
	return acl_dbuf_pool_memdup(pool_, addr, len);
}

bool dbuf_pool::dbuf_free(const void* addr)
{
	return acl_dbuf_pool_free(pool_, addr) == 0 ? true : false;
}

bool dbuf_pool::dbuf_keep(const void* addr)
{
	return acl_dbuf_pool_keep(pool_, addr) == 0 ? true : false;
}

bool dbuf_pool::dbuf_unkeep(const void* addr)
{
	return acl_dbuf_pool_unkeep(pool_, addr) == 0 ? true : false;
}

//////////////////////////////////////////////////////////////////////////////

dbuf_obj::dbuf_obj(dbuf_guard* guard /* = NULL */)
{
	if (guard)
		guard->push_back(this);
}

dbuf_guard::dbuf_guard(acl::dbuf_pool* dbuf /* = NULL */, size_t nblock /* = 2 */)
{
	if (dbuf == NULL)
		dbuf_ = new (nblock) acl::dbuf_pool;
	else
		dbuf_ = dbuf;
}

dbuf_guard::~dbuf_guard()
{
	for (std::vector<dbuf_obj*>::iterator it = objs_.begin();
		it != objs_.end(); ++it)
	{
		(*it)->~dbuf_obj();
	}

	dbuf_->destroy();
}

int dbuf_guard::push_back(dbuf_obj* obj)
{
	objs_.push_back(obj);
	return (int) objs_.size() - 1;
}

dbuf_obj* dbuf_guard::operator[](size_t pos) const
{
	if (pos >= objs_.size())
		return NULL;

	return objs_[pos];
}

} // namespace acl
