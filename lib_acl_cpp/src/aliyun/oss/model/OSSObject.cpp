#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/aliyun/oss/model/OSSObject.hpp"
#include "oss_macro.hpp"

namespace acl
{

OSSObject::OSSObject(dbuf_pool* pool /* = NULL */)
	: pool_(pool)
	, bucket_(NULL)
	, key_(NULL)
	, in_(NULL)
	, meta_(NULL)
	, info_(NULL)
{

}

OSSObject::~OSSObject()
{

}

OSSObject& OSSObject::setKey(const char* key)
{
	OSS_SANE_DUP(key, key_);
	return *this;
}

OSSObject& OSSObject::setBucketName(const char* name)
{
	OSS_SANE_DUP(name, bucket_);
	return *this;
}

OSSObject& OSSObject::setObjectContent(istream* in)
{
	in_ = in;
	return *this;
}

OSSObject& OSSObject::setObjectMetadata(ObjectMetadata* meta)
{
	meta_ = meta;
	return *this;
}

void OSSObject::reset()
{
	OSS_SANE_FREE(bucket_);
	OSS_SANE_FREE(key_);
	in_ = NULL;
	meta_ = NULL;
	delete info_;
	info_ = NULL;
}

const char* OSSObject::toString()
{
	if (info_ == NULL)
		info_ = NEW string(128);
	info_->format("OSSObject [key=%s,bucket=%s]",
		key_ ? key_ : "<Unknown>", bucket_ ? bucket_ : "<Unknown>");
	return info_->c_str();
}

} // namespace acl
