#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/aliyun/oss/model/CORSRule.hpp"

namespace acl
{

class istream;

class OSSBucketOperation;
class Bucket;

class OSSObjectOperation;
class OSSObject;
class CORSOperation;
class ObjectMetadata;
class OptionsRequest;
class PutObjectResult;
class CopyObjectResult;
class SetBucketCORSRequest;

class OSSMultipartOperation;
class AbortMultipartUploadRequest;
class CompleteMultipartUploadRequest;
class InitiateMultipartUploadRequest;
class ListMultipartUploadsRequest;
class ListPartsRequest;
class UploadPartRequest;
class UploadPartCopyRequest;
class CompleteMultipartUploadResult;
class InitiateMultipartUploadResult;
class MultipartUploadListing;
class PartListing;
class UploadPartResult;
class UploadPartCopyResult;

class ACL_CPP_API OSSClient
{
public:
	OSSClient(const char* keyId, const char* keySecret);
	OSSClient(const char* endPoint, const char* keyId,
		const char* keySecret);

	~OSSClient();

	const char* getEndpoint() const
	{
		return end_point_;
	}

	const char* getAccessKeyId() const
	{
		return key_id_;
	}

	const char* getAccessKeySecret() const
	{
		return key_secret_;
	}

	bool createBucket(Bucket& bucket);
	bool deleteBucket(const char* name);
	std::list<Bucket> listBuckets();
	const char* getBucketLocation(const char* name);
	bool doesBucketExist(const char* name);

	OSSObject* getObject(const char* bucket, const char* key);
	bool deleteObject(const char* bucket, const char* key);
	bool putObject(const char* bucket, const char* key, istream& in,
		ObjectMetadata& meta, PutObjectResult* result = NULL);
	bool copyObject(const char* fromBucket, const char* fromKey,
		const char* toBucket, const char* toKey,
		CopyObjectResult* result = NULL);
	bool getObjectMetadata(const char* bucket, const char* key,
		ObjectMetadata* result = NULL);

	bool setBucketCORS(SetBucketCORSRequest& request);
	bool deleteBucketCORSRules(const char* bucket);
	bool optionsObject(OptionsRequest& request);
	bool getBucketCORSRules(const char* bucket,
		std::list<CORSRule>* result);

	bool abortMultipartUpload(AbortMultipartUploadRequest& request);
	bool completeMultipartUpload(CompleteMultipartUploadRequest& request,
		CompleteMultipartUploadResult* result);
	bool initiateMultipartUpload(InitiateMultipartUploadRequest& request,
		InitiateMultipartUploadResult* result);
	bool listMultipartUploads(ListMultipartUploadsRequest& request,
		MultipartUploadListing* result);
	bool listParts(ListPartsRequest& request, PartListing* result);
	bool uploadPart(UploadPartRequest& request, UploadPartResult* result);
	bool uploadPartCopy(UploadPartCopyRequest& request,
		UploadPartCopyResult* result);

private:
	char* end_point_;
	char* key_id_;
	char* key_secret_;
	OSSBucketOperation* bucket_oper_;
	OSSObjectOperation* object_oper_;
	OSSMultipartOperation* multipart_oper_;
	CORSOperation* cors_oper_;
};

} // namespace acl
