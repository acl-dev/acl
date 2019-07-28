#pragma once
#include <list>
#include "acl_cpp/aliyun/oss/model/Bucket.hpp"

namespace acl
{

class OSSBucketOperation
{
public:
	OSSBucketOperation();
	~OSSBucketOperation();

	bool createBucket(Bucket& bucket);
	bool deleteBucket(const char* name);
	std::list<Bucket> listBuckets();
	const char* getBucketLocation(const char* name);
	bool doesBucketExist(const char* name);

private:
};

} // namespace acl
