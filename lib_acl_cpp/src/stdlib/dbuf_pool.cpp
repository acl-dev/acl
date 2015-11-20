#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
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
	: guard_(guard)
	, nrefer_(0)
	, pos_(-1)
{
	if (guard)
		guard->push_back(this);
}

dbuf_guard::dbuf_guard(acl::dbuf_pool* dbuf, size_t capacity /* = 100 */)
	: nblock_(2)
	, size_(0)
	, capacity_(capacity == 0 ? 100 : capacity)
	, incr_(100)
{
	if (dbuf == NULL)
		dbuf_ = new (nblock_) acl::dbuf_pool;
	else
		dbuf_ = dbuf;
	objs_ = (dbuf_obj**) dbuf_->dbuf_alloc(sizeof(dbuf_obj*) * capacity_);
	dbuf_->dbuf_keep(objs_);
}

dbuf_guard::dbuf_guard(size_t nblock /* = 2 */, size_t capacity /* = 100 */)
	: nblock_(nblock == 0 ? 2 : nblock)
	, size_(0)
	, capacity_(capacity == 0 ? 100 : capacity)
	, incr_(100)
{
	dbuf_ = new (nblock_) acl::dbuf_pool;
	objs_ = (dbuf_obj**) dbuf_->dbuf_alloc(sizeof(dbuf_obj*) * capacity_);
	dbuf_->dbuf_keep(objs_);
}

dbuf_guard::~dbuf_guard()
{
	for (size_t i = 0; i < size_; i++)
		objs_[i]->~dbuf_obj();

	dbuf_->destroy();
}

bool dbuf_guard::dbuf_reset(size_t reserve /* = 0 */)
{
	for (size_t i = 0; i < size_; i++)
		objs_[i]->~dbuf_obj();

	size_ = 0;
	return dbuf_->dbuf_reset(reserve);
}

void dbuf_guard::extend_objs()
{
	dbuf_obj** old_objs = objs_;
	capacity_ += incr_;

	objs_ = (dbuf_obj**) dbuf_->dbuf_alloc(sizeof(dbuf_obj*) * capacity_);

	for (size_t i = 0; i < size_; i++)
		objs_[i] = old_objs[i];

	dbuf_->dbuf_keep(objs_);
	dbuf_->dbuf_unkeep(old_objs);
}

void dbuf_guard::set_increment(size_t incr)
{
	if (incr > 0)
		incr_ = incr;
}

int dbuf_guard::push_back(dbuf_obj* obj)
{
	if (obj->nrefer_ < 1)
	{
		if (obj->guard_ == NULL)
			obj->guard_ = this;
		else if (obj->guard_ != this)
		{
			logger_fatal("obj->guard_(%p) != me(%p), nrefer: %d",
				obj->guard_, this, obj->nrefer_);
		}

		if (size_ >= capacity_)
			extend_objs();

		objs_[size_] = obj;
		obj->nrefer_++;
		obj->pos_ = (int) size_;
		size_++;
	}
	else if (obj->guard_ != this)
	{
		logger_fatal("obj->guard_(%p) != me(%p), nrefer: %d",
			obj->guard_, this, obj->nrefer_);
	}

	return obj->pos_;
}

dbuf_obj* dbuf_guard::operator[](size_t pos) const
{
	return get(pos);
}

dbuf_obj* dbuf_guard::get(size_t pos) const
{
	if (pos >= size_)
		return NULL;
	else
		return objs_[pos];
}

} // namespace acl
