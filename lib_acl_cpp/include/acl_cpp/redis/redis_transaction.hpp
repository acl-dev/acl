#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_transaction
{
public:
	redis_transaction(redis_client* conn = NULL);
	~redis_transaction();

	void reset();

	void set_client(redis_client* conn);
	redis_client* get_client() const
	{
		return conn_;
	}

	const redis_result* get_result() const
	{
		return result_;
	}

	/////////////////////////////////////////////////////////////////////

	bool watch(const std::vector<string>& keys);
	bool unwatch(const std::vector<string>& keys);
	bool multi();
	bool exec();
	bool discard();
	bool queue_cmd(const char* cmd, const char* argv[],
		const size_t lens[], size_t argc);

	size_t get_size();
	const redis_result* get_child(size_t i, string* cmd);

private:
	redis_client* conn_;
	const redis_result* result_;
	std::vector<string> cmds_;
};

} // namespace acl
