#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class dbuf_pool;

class ACL_CPP_API ResponseHeaderOverrides
{
public:
	ResponseHeaderOverrides(dbuf_pool* pool = NULL);
	~ResponseHeaderOverrides();

	void setContentType(const char* type);
	void setContentLangauge(const char* language);
	void setExpires(const char* expires);
	void setCacheControl(const char* control);
	void setContentDisposition(const char* disposition);
	void setContentEncoding(const char* encoding);
	void reset();

	const char* getContentType() const
	{
		return content_type_;
	}

	const char* getContentLangauge() const
	{
		return content_language_;
	}

	const char* getExpires() const
	{
		return expires_;
	}

	const char* getCacheControl() const
	{
		return cache_control_;
	}

	const char* getContentEncoding() const
	{
		return content_encoding_;
	}

private:
	dbuf_pool* pool_;
	dbuf_pool* pool_internal_;
	char* content_type_;
	char* content_language_;
	char* expires_;
	char* cache_control_;
	char* content_disposition_;
	char* content_encoding_;
};

} // namespace acl
