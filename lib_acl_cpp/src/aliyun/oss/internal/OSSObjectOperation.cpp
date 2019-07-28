#include "acl_stdafx.hpp"
#include "OSSObjectOperation.hpp"

namespace acl
{

OSSObjectOperation::OSSObjectOperation()
{

}

OSSObjectOperation::~OSSObjectOperation()
{

}

OSSObject* OSSObjectOperation::getObject(const char* bucket, const char* key)
{
	return NULL;
}

bool OSSObjectOperation::deleteObject(const char* bucket, const char* key)
{
	return false;
}

bool OSSObjectOperation::putObject(const char* bucket, const char* key,
	istream& in, ObjectMetadata& meta, PutObjectResult* result)
{
	return false;
}

bool OSSObjectOperation::copyObject(const char* fromBucket, const char* fromKey,
	const char* toBucket, const char* toKey, CopyObjectResult* result)
{
	return false;
}

bool OSSObjectOperation::getObjectMetadata(const char* bucket,
	const char* key, ObjectMetadata* result)
{
	return false;
}

} // namespace acl
