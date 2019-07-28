#include "acl_stdafx.hpp"
#include "acl_cpp/aliyun/oss/model/OptionsRequest.hpp"
#include "oss_macro.hpp"

namespace acl
{

OptionsRequest::OptionsRequest(dbuf_pool* pool /* = NULL */)
	: pool_(pool)
	, bucket_(NULL)
	, origin_(NULL)
	, headers_(NULL)
	, object_name_(NULL)
	, method_(HTTP_METHOD_UNKNOWN)
{

}

OptionsRequest::~OptionsRequest()
{
	reset();
}

OptionsRequest& OptionsRequest::setBucketName(const char* name)
{
	OSS_SANE_DUP(name, bucket_);
	return *this;
}

OptionsRequest& OptionsRequest::setOrigin(const char* origin)
{
	OSS_SANE_DUP(origin, origin_);
	return *this;
}

OptionsRequest& OptionsRequest::setRequestMethod(http_method_t method)
{
	method_ = method;
	return *this;
}

OptionsRequest& OptionsRequest::setRequestHeaders(const char* headers)
{
	OSS_SANE_DUP(headers, headers_);
	return *this;
}

OptionsRequest& OptionsRequest::setObjectName(const char* name)
{
	OSS_SANE_DUP(name, object_name_);
	return *this;
}

void OptionsRequest::reset()
{
	OSS_SANE_FREE(bucket_);
	OSS_SANE_FREE(origin_);
	OSS_SANE_FREE(headers_);
	OSS_SANE_FREE(object_name_);
	method_ = HTTP_METHOD_UNKNOWN;
}

} // namespace acl
