#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/aliyun/oss/model/Bucket.hpp"
#include "oss_macro.hpp"

namespace acl
{

Bucket::Bucket(const char* name, const char* owner, dbuf_pool* pool /* = NULL */)
	: pool_(pool)
	, name_(NULL)
	, owner_(NULL)
	, location_(NULL)
	, creation_date_(0)
	, info_(NULL)
{
	acl_assert(name && *name);
	acl_assert(owner && *owner);
	setName(name);
	setOwner(owner);
}

Bucket::~Bucket()
{
	reset();
}

Bucket& Bucket::setName(const char* name)
{
	OSS_SANE_DUP(name, name_);
	return *this;
}

Bucket& Bucket::setOwner(const char* owner)
{
	OSS_SANE_DUP(owner, owner_);
	return *this;
}

Bucket& Bucket::setLocation(const char* location)
{
	OSS_SANE_DUP(location, location_);
	return *this;
}

Bucket& Bucket::setCreationDate(time_t creationDate)
{
	creation_date_ = creationDate;
	return *this;
}

const char* Bucket::toString()
{
	if (info_ == NULL)
		info_ = NEW string(128);
	info_->format("OSSBucket [name=%s, creationDate=%ld, owner=%s]",
		name_ ? name_ : "<Unknown>", creation_date_,
		owner_ ? owner_ : "<Unknown>");
	return info_->c_str();
}

void Bucket::reset()
{
	OSS_SANE_FREE(name_);
	OSS_SANE_FREE(owner_);
	OSS_SANE_FREE(location_);
	delete info_;
	info_ = NULL;
	creation_date_ = 0;
}

} // namespace acl
