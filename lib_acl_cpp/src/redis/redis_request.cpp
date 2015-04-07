#include "acl_stdafx.hpp"
#include "redis_request.hpp"

namespace acl
{

redis_request::redis_request()
: iov_(NULL)
, argc_(0)
, size_(0)
{
}

redis_request::~redis_request()
{
	if (iov_ != NULL)
		acl_myfree(iov_);
}

void redis_request::clear()
{
	argc_ = 0;
}

void redis_request::reserve(size_t size)
{
	if (size_ >= size)
		return;

	size_t len = size * sizeof(struct iovec);

	if (iov_ == NULL)
		iov_ = (struct iovec*) acl_mymalloc(len);
	else
		iov_ = (struct iovec*) acl_myrealloc(iov_, len);

	size_ = size;
}

void redis_request::put(const char* data, size_t dlen)
{
	iov_[argc_].iov_base = (void*) data;
	iov_[argc_].iov_len = dlen;
	argc_++;
}

} // namespace acl
