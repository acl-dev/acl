#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

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

class ACL_CPP_API OSSMultipartOperation
{
public:
	OSSMultipartOperation();
	~OSSMultipartOperation();

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
};

} // namespace acl
