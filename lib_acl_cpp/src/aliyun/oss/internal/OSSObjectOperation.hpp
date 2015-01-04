#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

class istream;
class OSSObject;
class ObjectMetadata;
class PutObjectResult;
class CopyObjectResult;

class OSSObjectOperation
{
public:
	OSSObjectOperation();
	~OSSObjectOperation();

	OSSObject* getObject(const char* bucket, const char* key);
	bool deleteObject(const char* bucket, const char* key);
	bool putObject(const char* bucket, const char* key, istream& in,
		ObjectMetadata& meta, PutObjectResult* result);
	bool copyObject(const char* fromBucket, const char* fromKey,
		const char* toBucket, const char* toKey,
		CopyObjectResult* result);
	bool getObjectMetadata(const char* bucket, const char* key,
		ObjectMetadata* result);
};

} // namespace acl
