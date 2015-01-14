#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_list
{
public:
	redis_list(redis_client& conn);
	~redis_list();

	redis_client& get_client() const
	{
		return conn_;
	}

private:
	redis_client& conn_;
};

} // namespace acl
