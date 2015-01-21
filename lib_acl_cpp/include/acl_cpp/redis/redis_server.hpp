#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_server : public redis_command
{
public:
	redis_server(redis_client* conn = NULL);
	~redis_server();
};

} // namespace acl
