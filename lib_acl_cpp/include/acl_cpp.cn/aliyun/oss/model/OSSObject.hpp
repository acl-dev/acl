#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class dbuf_pool;
class istream;
class string;
class ObjectMetadata;

class ACL_CPP_API OSSObject
{
public:
	OSSObject(dbuf_pool* pool = NULL);
	~OSSObject();

	OSSObject& setKey(const char* key);
	OSSObject& setBucketName(const char* name);
	OSSObject& setObjectContent(istream* in);
	OSSObject& setObjectMetadata(ObjectMetadata* meta);
	void reset();

	const char* getKey() const
	{
		return key_;
	}

	const char* getBucketName() const
	{
		return bucket_;
	}

	istream* getObjectContent()
	{
		return in_;
	}

	ObjectMetadata* getObjectMetadata() const
	{
		return meta_;
	}

	const char* toString();

private:
	dbuf_pool* pool_;
	char* bucket_;
	char* key_;
	istream* in_;
	ObjectMetadata* meta_;
	string* info_;
};

} // namespace acl
