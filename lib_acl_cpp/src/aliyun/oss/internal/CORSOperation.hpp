#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <list>
#include "acl_cpp/aliyun/oss/model/CORSRule.hpp"

namespace acl
{

class SetBucketCORSRequest;
class OptionsRequest;

class ACL_CPP_API CORSOperation
{
public:
	CORSOperation();
	~CORSOperation();

	bool setBucketCORS(SetBucketCORSRequest& request);
	bool deleteBucketCORSRules(const char* bucket);
	bool optionsObject(OptionsRequest& request);
	bool getBucketCORSRules(const char* bucket,
		std::list<CORSRule>* result);

private:
};

} // namespace acl
