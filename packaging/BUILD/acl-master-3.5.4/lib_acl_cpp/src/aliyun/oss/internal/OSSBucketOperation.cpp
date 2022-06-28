#include "acl_stdafx.hpp"
#include "OSSBucketOperation.hpp"

namespace acl
{

OSSBucketOperation::OSSBucketOperation()
{

}

OSSBucketOperation::~OSSBucketOperation()
{

}

bool OSSBucketOperation::createBucket(Bucket& bucket)
{
	return false;
}

bool OSSBucketOperation::deleteBucket(const char* name)
{
	return false;
}

std::list<Bucket> OSSBucketOperation::listBuckets()
{
	std::list<Bucket> buckets;
	return buckets;
}

const char* OSSBucketOperation::getBucketLocation(const char* name)
{
	return NULL;
}

bool OSSBucketOperation::doesBucketExist(const char* name)
{
	return false;
}

} // namespace acl
