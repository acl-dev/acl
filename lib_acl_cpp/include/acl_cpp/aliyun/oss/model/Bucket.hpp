#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class dbuf_pool;

class ACL_CPP_API Bucket
{
public:
	Bucket(const char* name, const char* owner, dbuf_pool* pool = NULL);
	~Bucket();

	Bucket& setName(const char* name);
	Bucket& setOwner(const char* owner);
	Bucket& setLocation(const char* location);
	Bucket& setCreationDate(time_t creationDate);
	void reset();

	const char* getName() const
	{
		return name_;
	}

	const char* getOwner() const
	{
		return owner_;
	}

	const char* getLocation() const
	{
		return location_;
	}

	time_t getCreationDate() const
	{
		return creation_date_;
	}

	const char* toString();

private:
	dbuf_pool* pool_;
	char* name_;
	char* owner_;
	char* location_;
	time_t creation_date_;
	string* info_;;
};

} // namespace acl
