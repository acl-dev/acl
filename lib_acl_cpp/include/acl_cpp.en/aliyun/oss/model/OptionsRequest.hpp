#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/http/http_type.hpp"

namespace acl
{

class dbuf_pool;

class ACL_CPP_API OptionsRequest
{
public:
	OptionsRequest(dbuf_pool* pool = NULL);
	~OptionsRequest();

	OptionsRequest& setBucketName(const char* name);
	OptionsRequest& setOrigin(const char* origin);
	OptionsRequest& setRequestMethod(http_method_t method);
	OptionsRequest& setRequestHeaders(const char* headers);
	OptionsRequest& setObjectName(const char* name);
	void reset();

	const char* getBucketName() const
	{
		return bucket_;
	}

	const char* getOrigin() const
	{
		return origin_;
	}

	http_method_t getRequestMethod() const
	{
		return method_;
	}

	const char* getRequestHeaders() const
	{
		return headers_;
	}

	const char* getObjectName() const
	{
		return object_name_;
	}

private:
	dbuf_pool* pool_;
	char* bucket_;
	char* origin_;
	char* headers_;
	char* object_name_;
	http_method_t method_;
};

} // namespace acl
