#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class ACL_CPP_API ObjectMetadata
{
public:
	ObjectMetadata();
	~ObjectMetadata();

private:
	std::map<string, string> meta_data_;
};

} // namespace acl
