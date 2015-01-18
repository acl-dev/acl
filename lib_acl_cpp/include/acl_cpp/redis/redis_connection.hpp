#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_connection
{
public:
	redis_connection(redis_client* conn = NULL);
	~redis_connection();

	const redis_result* get_result() const
	{
		return result_;
	}

	void set_client(redis_client* conn);
	redis_client* get_client() const
	{
		return conn_;
	}

	/////////////////////////////////////////////////////////////////////

	bool auth(const char* passwd);
	bool select(int dbnum);
	bool ping();
	bool echo(const char* s);
	bool quit();

private:
	redis_client* conn_;
	const redis_result* result_;
};

} // namespace acl
