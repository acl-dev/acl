#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_connection : public redis_command
{
public:
	redis_connection(redis_client* conn = NULL);
	~redis_connection();

	/////////////////////////////////////////////////////////////////////

	bool auth(const char* passwd);
	bool select(int dbnum);
	bool ping();
	bool echo(const char* s);
	bool quit();
};

} // namespace acl
