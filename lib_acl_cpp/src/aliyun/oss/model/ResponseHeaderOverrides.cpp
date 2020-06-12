#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/aliyun/oss/model/ResponseHeaderOverrides.hpp"

namespace acl
{

ResponseHeaderOverrides::ResponseHeaderOverrides(dbuf_pool* pool /* = NULL */)
	: pool_(pool)
	, pool_internal_(NULL)
	, content_type_(NULL)
	, content_language_(NULL)
	, expires_(NULL)
	, cache_control_(NULL)
	, content_disposition_(NULL)
	, content_encoding_(NULL)
{

}

ResponseHeaderOverrides::~ResponseHeaderOverrides()
{
	delete pool_internal_;
}

#define CHECK_POOL() do \
{ \
	if (pool_ == NULL) \
	{ \
		pool_internal_ = new dbuf_pool(); \
		pool_ = pool_internal_; \
	} \
} while (0)

void ResponseHeaderOverrides::setContentType(const char* type)
{
	if (type == NULL || *type == 0)
		return;

	CHECK_POOL();
	content_type_ = pool_->dbuf_strdup(type);
}

void ResponseHeaderOverrides::setContentLangauge(const char* language)
{
	if (language == NULL || *language == 0)
		return;

	CHECK_POOL();
	content_language_ = pool_->dbuf_strdup(language);
}

void ResponseHeaderOverrides::setExpires(const char* expires)
{
	if (expires == NULL || *expires == 0)
		return;

	CHECK_POOL();
	expires_ = pool_->dbuf_strdup(expires);
}

void ResponseHeaderOverrides::setCacheControl(const char* control)
{
	if (control == NULL || *control == 0)
		return;

	CHECK_POOL();
	cache_control_ = pool_->dbuf_strdup(control);
}

void ResponseHeaderOverrides::setContentDisposition(const char* disposition)
{
	if (disposition == NULL || *disposition == 0)
		return;

	CHECK_POOL();
	content_disposition_ = pool_->dbuf_strdup(disposition);
}

void ResponseHeaderOverrides::setContentEncoding(const char* encoding)
{
	if (encoding == NULL || *encoding == 0)
		return;

	CHECK_POOL();
	content_encoding_ = pool_->dbuf_strdup(encoding);
}

void ResponseHeaderOverrides::reset()
{
	if (pool_ == pool_internal_)
		pool_ = NULL;
	delete pool_internal_;
	pool_internal_ = NULL;

	content_type_ = NULL;
	content_language_ = NULL;
	expires_ = NULL;
	cache_control_ = NULL;
	content_disposition_ = NULL;
	content_encoding_ = NULL;
}

} // namespace acl
