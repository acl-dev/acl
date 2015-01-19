#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_transaction : public redis_command
{
public:
	redis_transaction(redis_client* conn = NULL);
	~redis_transaction();

	/////////////////////////////////////////////////////////////////////

	bool watch(const std::vector<string>& keys);
	bool unwatch(const std::vector<string>& keys);
	bool multi();
	bool exec();
	bool discard();
	bool queue_cmd(const char* cmd, const char* argv[],
		const size_t lens[], size_t argc);

	size_t get_size() const;
	const redis_result* get_child(size_t i, string* cmd) const;

private:
	std::vector<string> cmds_;
};

} // namespace acl
