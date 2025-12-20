#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class dbuf_pool;

class ACL_CPP_API CopyObjectResult
{
public:
	CopyObjectResult(dbuf_pool* pool = NULL);
	~CopyObjectResult();

	CopyObjectResult& setEtag(const char* etag);
	CopyObjectResult& setLastModified(time_t lastModified);
	void reset();

	const char* getEtag() const
	{
		return etag_;
	}

	time_t getLastModified() const
	{
		return last_modified_;
	}

private:
	dbuf_pool* pool_;
	char* etag_;
	time_t last_modified_;
};

} // namespace acl

