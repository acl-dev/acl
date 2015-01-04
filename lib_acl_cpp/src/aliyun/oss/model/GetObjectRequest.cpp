#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/aliyun/oss/model/GetObjectRequest.hpp"
#include "oss_macro.hpp"

namespace acl
{

GetObjectRequest::GetObjectRequest(const char* bucket, const char* key,
	dbuf_pool* pool /* = NULL */)
	: pool_(pool)
	, bucket_(NULL)
	, key_(NULL)
	, unmodified_since_(0)
	, modified_since_(0)
	, response_headers_(NULL)
{
	setBucketName(bucket);
	setKey(key);
	setRange(0, 0);
}

GetObjectRequest::~GetObjectRequest()
{
	reset();
}

GetObjectRequest& GetObjectRequest::setBucketName(const char* name)
{
	OSS_SANE_DUP(name, bucket_);
	return *this;
}

GetObjectRequest& GetObjectRequest::setKey(const char* key)
{
	OSS_SANE_DUP(key, key_);
	return *this;
}

GetObjectRequest& GetObjectRequest::setRange(
	long long int start, long long int end)
{
	if (start < 0 && end < 0)
	{
		logger_error("invalid start: %lld, end: %lld", start, end);
		return *this;
	}
	range_[0] = start;
	range_[1] = end;
	return *this;
}

GetObjectRequest& GetObjectRequest::setMatchingETagConstraints(
	const std::list<string>& etags)
{
	OSS_FREE_LIST(matching_etags_);
	OSS_COPY_LIST(etags, matching_etags_);
	return *this;
}

GetObjectRequest& GetObjectRequest::setNonmatchingETagConstraints(
	const std::list<string>& etags)
{
	OSS_FREE_LIST(non_matching_etags_);
	OSS_COPY_LIST(etags, non_matching_etags_);
	return *this;
}

GetObjectRequest& GetObjectRequest::setUnmodifiedSinceConstraint(time_t date)
{
	unmodified_since_ = date;
	return *this;
}

GetObjectRequest& GetObjectRequest::setModifiedSinceConstraint(time_t date)
{
	modified_since_ = date;
	return *this;
}

GetObjectRequest& GetObjectRequest::setResponseHeaders(
	ResponseHeaderOverrides* responseHeaders)
{
	response_headers_ = responseHeaders;
	return *this;
}

void GetObjectRequest::reset()
{
	OSS_SANE_FREE(bucket_);
	OSS_SANE_FREE(key_);
	setRange(0, 0);
	OSS_FREE_LIST(matching_etags_);
	OSS_FREE_LIST(non_matching_etags_);
	unmodified_since_ = 0;
	modified_since_ = 0;
	response_headers_ = NULL;
}

} // namespace acl
