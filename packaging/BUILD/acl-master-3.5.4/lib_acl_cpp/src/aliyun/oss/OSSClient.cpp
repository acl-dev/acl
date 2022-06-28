#include "acl_stdafx.hpp"
#include "OSSConstants.hpp"
#include "acl_cpp/aliyun/oss/OSSClient.hpp"

#include "internal/OSSBucketOperation.hpp"
#include "internal/OSSObjectOperation.hpp"
#include "internal/OSSMultipartOperation.hpp"
#include "internal/CORSOperation.hpp"

namespace acl
{

OSSClient::OSSClient(const char* key_id, const char* key_secret)
{
	OSSClient(DEFAULT_OSS_ENDPOINT, key_id, key_secret);
}

OSSClient::OSSClient(const char* end_point, const char* key_id,
	const char* key_secret)
{
	acl_assert(end_point && *end_point);
	acl_assert(key_id_ && *key_id_);
	acl_assert(key_secret && *key_secret);

	end_point_ = acl_mystrdup(end_point);
	key_id_ = acl_mystrdup(key_id);
	key_secret_ = acl_mystrdup(key_secret);

	bucket_oper_ = NULL;
	object_oper_ = NULL;
	multipart_oper_ = NULL;
	cors_oper_ = NULL;
}

OSSClient::~OSSClient()
{
	acl_myfree(end_point_);
	acl_myfree(key_id_);
	acl_myfree(key_secret_);

	delete bucket_oper_;
	delete object_oper_;
	delete multipart_oper_;
	delete cors_oper_;
}

bool OSSClient::createBucket(Bucket& bucket)
{
	if (bucket_oper_ == NULL)
		bucket_oper_ = NEW OSSBucketOperation();
	return bucket_oper_->createBucket(bucket);
}

bool OSSClient::deleteBucket(const char* name)
{
	if (bucket_oper_ == NULL)
		bucket_oper_ = NEW OSSBucketOperation();
	return bucket_oper_->deleteBucket(name);
}

std::list<Bucket> OSSClient::listBuckets()
{
	if (bucket_oper_ == NULL)
		bucket_oper_ = NEW OSSBucketOperation();
	return bucket_oper_->listBuckets();
}

const char* OSSClient::getBucketLocation(const char* name)
{
	if (bucket_oper_ == NULL)
		bucket_oper_ = NEW OSSBucketOperation();
	return bucket_oper_->getBucketLocation(name);
}

bool OSSClient::doesBucketExist(const char* name)
{
	if (bucket_oper_ == NULL)
		bucket_oper_ = NEW OSSBucketOperation();
	return bucket_oper_->doesBucketExist(name);
}

OSSObject* OSSClient::getObject(const char* bucket, const char* key)
{
	if (object_oper_ == NULL)
		object_oper_ = NEW OSSObjectOperation();
	return object_oper_->getObject(bucket, key);
}

bool OSSClient::deleteObject(const char* bucket, const char* key)
{
	if (object_oper_ == NULL)
		object_oper_ = NEW OSSObjectOperation();
	return object_oper_->deleteObject(bucket, key);
}

bool OSSClient::putObject(const char* bucket, const char* key, istream& in,
       ObjectMetadata& meta, PutObjectResult* result /* = NULL */)
{
	if (object_oper_ == NULL)
		object_oper_ = NEW OSSObjectOperation();
	return object_oper_->putObject(bucket, key, in, meta, result);
}

bool OSSClient::copyObject(const char* fromBucket, const char* fromKey,
	const char* toBucket, const char* toKey,
	CopyObjectResult* result /* = NULL */)
{
	if (object_oper_ == NULL)
		object_oper_ = NEW OSSObjectOperation();
	return object_oper_->copyObject(fromBucket, fromKey,
		toBucket, toKey, result);
}

bool OSSClient::getObjectMetadata(const char* bucket, const char* key,
	ObjectMetadata* result /* = NULL */)
{
	if (object_oper_ == NULL)
		object_oper_ = NEW OSSObjectOperation();
	return object_oper_->getObjectMetadata(bucket, key, result);
}

bool OSSClient::setBucketCORS(SetBucketCORSRequest& request)
{
	if (cors_oper_ == NULL)
		cors_oper_ = NEW CORSOperation();
	return cors_oper_->setBucketCORS(request);
}

bool OSSClient::deleteBucketCORSRules(const char* bucket)
{
	if (cors_oper_ == NULL)
		cors_oper_ = NEW CORSOperation();
	return cors_oper_->deleteBucketCORSRules(bucket);
}

bool OSSClient::optionsObject(OptionsRequest& request)
{
	if (cors_oper_ == NULL)
		cors_oper_ = NEW CORSOperation();
	return cors_oper_->optionsObject(request);
}

bool OSSClient::getBucketCORSRules(const char* bucket,
	std::list<CORSRule>* result)
{
	if (cors_oper_ == NULL)
		cors_oper_ = NEW CORSOperation();
	return cors_oper_->getBucketCORSRules(bucket, result);
}

bool OSSClient::abortMultipartUpload(AbortMultipartUploadRequest& request)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->abortMultipartUpload(request);
}

bool OSSClient::completeMultipartUpload(CompleteMultipartUploadRequest& request,
	CompleteMultipartUploadResult* result)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->completeMultipartUpload(request, result);
}

bool OSSClient::initiateMultipartUpload(InitiateMultipartUploadRequest& request,
	InitiateMultipartUploadResult* result)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->initiateMultipartUpload(request, result);
}

bool OSSClient::listMultipartUploads(ListMultipartUploadsRequest& request,
	MultipartUploadListing* result)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->listMultipartUploads(request, result);
}

bool OSSClient::listParts(ListPartsRequest& request, PartListing* result)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->listParts(request, result);
}

bool OSSClient::uploadPart(UploadPartRequest& request,
	UploadPartResult* result)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->uploadPart(request, result);
}

bool OSSClient::uploadPartCopy(UploadPartCopyRequest& request,
	UploadPartCopyResult* result)
{
	if (multipart_oper_ == NULL)
		multipart_oper_ = NEW OSSMultipartOperation();
	return multipart_oper_->uploadPartCopy(request, result);
}

} // namespace acl
