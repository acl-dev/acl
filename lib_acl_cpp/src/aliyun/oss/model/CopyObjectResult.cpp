#include "acl_stdafx.hpp"
#include "acl_cpp/aliyun/oss/model/CopyObjectResult.hpp"
#include "oss_macro.hpp"

namespace acl
{

CopyObjectResult::CopyObjectResult(dbuf_pool* pool /* = NULL */)
	: pool_(pool)
	, etag_(NULL)
	, last_modified_(0)
{

}

CopyObjectResult::~CopyObjectResult()
{
	reset();
}

CopyObjectResult& CopyObjectResult::setEtag(const char* etag)
{
	OSS_SANE_DUP(etag, etag_);
	return *this;
}

CopyObjectResult& CopyObjectResult::setLastModified(time_t lastModified)
{
	last_modified_ = lastModified;
	return *this;
}

void CopyObjectResult::reset()
{
	OSS_SANE_FREE(etag_);
}

} // namespace acl
