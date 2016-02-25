#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#endif

namespace acl
{

redis_result::redis_result(dbuf_pool* dbuf)
: result_type_(REDIS_RESULT_NIL)
, dbuf_(dbuf)
, size_(0)
, idx_(0)
, argv_(NULL)
, lens_(NULL)
, children_(NULL)
, children_size_(10)
, children_idx_(0)
{
	acl_assert(dbuf_ != NULL);
}

redis_result::~redis_result()
{
}

void *redis_result::operator new(size_t size, dbuf_pool* pool)
{
	return pool->dbuf_alloc(size);
}

void redis_result::operator delete(void* ptr acl_unused,
	dbuf_pool* pool acl_unused)
{
	logger_error("DELETE NOW!");
}

void redis_result::clear()
{
	children_ = NULL;
	children_idx_ = 0;
}

redis_result& redis_result::set_size(size_t size)
{
	if (idx_ > 0)
	{
		logger_error("set size when putting, idx_: %d", (int) idx_);
		return *this;
	}

	size_ = size;
	return *this;
}

redis_result& redis_result::set_type(redis_result_t type)
{
	result_type_ = type;
	return *this;
}

redis_result& redis_result::put(const char* buf, size_t len)
{
	if (size_ == 0)
	{
		logger_error("size_ is 0, call set_size first!");
		return *this;
	}
	if (idx_ >= size_)
	{
		logger_error("overflow, idx_(%d) >= size_(%d)",
			(int) idx_, (int) size_);
		return *this;
	}
	if (argv_ == NULL)
	{
		argv_ = (const char**) dbuf_->dbuf_alloc(sizeof(char*) * size_);
		lens_ = (size_t*) dbuf_->dbuf_alloc(sizeof(size_t) * size_);
	}

	argv_[idx_] = buf;
	lens_[idx_] = len;
	idx_++;

	return *this;
}

size_t redis_result::get_size() const
{
	if (result_type_ == REDIS_RESULT_ARRAY)
		return children_idx_;
	else if (result_type_ == REDIS_RESULT_STRING)
	{
		if (argv_ == NULL || lens_ == NULL)
			return 0;
		return size_;
	}
	else
		return size_;
}

int redis_result::get_integer(bool* success /* = NULL */) const
{
	if (success)
		*success = false;
	if (result_type_ != REDIS_RESULT_INTEGER)
		return -1;
	const char* ptr = get(0);
	if (ptr == NULL || *ptr == 0)
		return -1;
	if (success)
		*success = true;
	return atoi(ptr);
}

long long int redis_result::get_integer64(bool* success /* = NULL */) const
{
	if (success)
		*success = false;
	if (result_type_ != REDIS_RESULT_INTEGER)
		return -1;
	const char* ptr = get(0);
	if (ptr == NULL || *ptr == 0)
		return -1;
	if (success)
		*success = true;
	return acl_atoi64(ptr);
}

double redis_result::get_double(bool* success /* = NULL */) const
{
	if (success)
		*success = false;
	if (result_type_ != REDIS_RESULT_STRING)
		return -1;
	const char* ptr = get(0);
	if (ptr == NULL || *ptr == 0)
		return -1;
	if (success)
		*success = true;
	return atof(ptr);
}

const char* redis_result::get_status() const
{
	if (result_type_ != REDIS_RESULT_STATUS)
		return "";
	const char* ptr = get(0);
	return ptr == NULL ? "" : ptr;
}

const char* redis_result::get_error() const
{
	if (result_type_ != REDIS_RESULT_ERROR)
		return "";
	const char* ptr = get(0);
	return ptr == NULL ? "" : ptr;
}

const char* redis_result::get(size_t i, size_t* len /* = NULL */) const
{
	if (i >= idx_)
	{
		if (len)
			*len = 0;
		return NULL;
	}
	if (len)
		*len = lens_[i];
	return argv_[i];
}

size_t redis_result::get_length() const
{
	if (lens_ == NULL)
		return 0;

	size_t len = 0;
	for (size_t i = 0; i < idx_; i++)
		len += lens_[i];
	return len;
}

int redis_result::argv_to_string(string& buf) const
{
	buf.clear();

	if (idx_ == 0)
		return -1;

	int length = 0;
	for (size_t i = 0; i < idx_; i++)
	{
		buf.append(argv_[i], lens_[i]);
		length += (int) lens_[i];
	}

	return length;
}

int redis_result::argv_to_string(char* buf, size_t size) const
{
	if (idx_ == 0 || size == 0)
		return -1;

	size--;
	if (size == 0)
		return 0;

	char* ptr = buf;
	int length = 0;
	size_t n;
	for (size_t i = 0; i < idx_; i++)
	{
		n = size > lens_[i] ? lens_[i] : size;
		memcpy(ptr, argv_[i], n);
		ptr += n;
		size -= n;
		length += (int) n;
		if (size == 0)
			break;
	}

	*ptr = 0;
	return length;
}

redis_result& redis_result::put(const redis_result* rr, size_t idx)
{
	if (children_ == NULL)
		children_ = (const redis_result**) dbuf_->dbuf_alloc(
				sizeof(redis_result*) * children_size_);
	else if (idx == 0)
		children_idx_ = 0;

	// +1 是为了确保最后一个数组元素可以被设为 NULL
	if (children_idx_ + 1 < children_size_)
	{
		children_[children_idx_++] = rr;
		return *this;
	}

	children_size_ *= 2;
	const redis_result** children =(const redis_result**)
	       	dbuf_->dbuf_calloc(sizeof(redis_result*) * children_size_);

	for (size_t i = 0; i < children_idx_; i++)
		children[i] = children_[i];

	children_ = children;
	children_[children_idx_++] = rr;

	return *this;
}

const redis_result* redis_result::get_child(size_t i) const
{
	if (children_ == NULL || i >= children_idx_)
		return NULL;
	return children_[i];
}

const redis_result** redis_result::get_children(size_t* size) const
{
	if (size)
		*size = children_idx_;
	return children_;
}

const string& redis_result::to_string(string& out) const
{
	redis_result_t type = get_type();
	if (type != REDIS_RESULT_ARRAY)
	{
		string buf;
		argv_to_string(buf);
		out += buf;
		out += "\r\n";
		return out;
	}

	size_t size;
	const redis_result** children = get_children(&size);
	if (children == NULL)
		return out;

	for (size_t i = 0; i < size; i++)
	{
		const redis_result* rr = children[i];
		if (rr != NULL)
			rr->to_string(out);
	}

	return out;
}

} // namespace acl
