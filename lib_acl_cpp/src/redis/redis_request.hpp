#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

struct iovec;

namespace acl
{

class ACL_CPP_API redis_request
{
public:
	redis_request();
	~redis_request();

	void clear();
	void reserve(size_t size);
	void put(const char* data, size_t dlen);

	struct iovec* get_iovec() const
	{
		return iov_;
	}

	size_t get_size() const
	{
		return argc_;
	}

private:
	struct iovec* iov_;
	size_t  argc_;
	size_t  size_;
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
