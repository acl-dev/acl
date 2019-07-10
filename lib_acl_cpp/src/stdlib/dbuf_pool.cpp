#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#endif

namespace acl
{

dbuf_pool::dbuf_pool(void)
{
}

dbuf_pool::~dbuf_pool(void)
{
}

void dbuf_pool::destroy(void)
{
	delete this;
}

void *dbuf_pool::operator new(size_t size, size_t nblock /* = 2 */)
{
	if (nblock == 0) {
		nblock = 2;
	}
	ACL_DBUF_POOL* pool = acl_dbuf_pool_create(4096 * nblock);
	dbuf_pool* dbuf     = (dbuf_pool*) acl_dbuf_pool_alloc(pool, size);
	dbuf->pool_         = pool;
	dbuf->mysize_       = size;

	return dbuf;
}

#if defined(_WIN32) || defined(_WIN64)
void dbuf_pool::operator delete(void* ptr, size_t)
{
	dbuf_pool* dbuf = (dbuf_pool*) ptr;
	acl_dbuf_pool_destroy(dbuf->pool_);
}
#endif

void dbuf_pool::operator delete(void* ptr)
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
	if (guard) {
		guard->push_back(this);
	}
}

void dbuf_guard::init(size_t capacity)
{
	if (capacity == 0) {
		capacity = 500;
	}
	head_.capacity = capacity;
	head_.size = 0;
	head_.next = NULL;
	head_.objs = (dbuf_obj**)
		dbuf_->dbuf_alloc(sizeof(dbuf_obj*) * capacity);
	dbuf_->dbuf_keep(head_.objs);
	curr_ = &head_;
}

dbuf_guard::dbuf_guard(acl::dbuf_pool* dbuf, size_t capacity /* = 500 */)
: nblock_(2)
, incr_(500)
, size_(0)
{
	if (dbuf == NULL) {
		dbuf_ = new (nblock_) acl::dbuf_pool;
	} else {
		dbuf_ = dbuf;
	}

	init(capacity);
}

dbuf_guard::dbuf_guard(size_t nblock /* = 2 */, size_t capacity /* = 500 */)
: nblock_(nblock == 0 ? 2 : nblock)
, incr_(500)
, size_(0)
{
	dbuf_ = new (nblock_) acl::dbuf_pool;
	init(capacity);
}

dbuf_guard::~dbuf_guard(void)
{
	dbuf_objs_link* link = &head_;

	while (link != NULL) {
		for (size_t i = 0; i < link->size; i++) {
			link->objs[i]->~dbuf_obj();
		}

		link = link->next;
	}

	dbuf_->destroy();
}

bool dbuf_guard::dbuf_reset(size_t reserve /* = 0 */)
{
	dbuf_objs_link* link = &head_;

	while (link != NULL) {
		for (size_t i = 0; i < link->size; i++) {
			link->objs[i]->~dbuf_obj();
		}
		link->size = 0;
		link = link->next;
	}

	head_.next = NULL;
	curr_ = &head_;
	size_ = 0;

	return dbuf_->dbuf_reset(reserve);
}

void dbuf_guard::extend_objs(void)
{
	dbuf_objs_link* link = (dbuf_objs_link*)
		dbuf_->dbuf_alloc(sizeof(dbuf_objs_link));

	link->capacity = incr_;
	link->size = 0;
	link->next = NULL;
	link->objs = (dbuf_obj**) dbuf_->dbuf_alloc(sizeof(dbuf_obj*) * incr_);
	curr_->next = link;
	curr_ = link;
}

void dbuf_guard::set_increment(size_t incr)
{
	if (incr > 0) {
		incr_ = incr;
	}
}

int dbuf_guard::push_back(dbuf_obj* obj)
{
	if (obj->nrefer_ < 1) {
		if (obj->guard_ == NULL) {
			obj->guard_ = this;
		} else if (obj->guard_ != this) {
			logger_fatal("obj->guard_(%p) != me(%p), nrefer: %d",
				obj->guard_, this, obj->nrefer_);
		}

		if (curr_->size >= curr_->capacity) {
			extend_objs();
		}

		curr_->objs[curr_->size] = obj;
		obj->nrefer_++;
		obj->pos_ = (int) size_++;
		curr_->size++;
	} else if (obj->guard_ != this) {
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
	if (pos >= size_) {
		return NULL;
	}

	size_t n = 0;
	const dbuf_objs_link* link = &head_;

	while (link != NULL) {
		if (pos >= n && pos < n + link->size) {
			return link->objs[pos - n];
		}

		n += link->size;
		link = link->next;
	}

	return NULL;
}

} // namespace acl
