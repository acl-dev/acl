#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class ACL_CPP_API PutObjectResult
{
public:
	PutObjectResult();
	~PutObjectResult();

	PutObjectResult& setETag(const char* etag);
	const char* getETag() const
	{
		return etag_.c_str();
	}

private:
	string etag_;
};

} // namespace acl

