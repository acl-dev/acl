#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_set : public redis_command
{
public:
	redis_set(redis_client* conn = NULL);
	~redis_set();

	/////////////////////////////////////////////////////////////////////

	int sadd(const char* key, const char* first_member, ...);
	int sadd(const char* key, const std::vector<const char*>& memsbers);
	int sadd(const char* key, const std::vector<string>& members);
	int sadd(const char* key, const char* argv[], size_t argc);
	int sadd(const char* key, const char* argv[], const size_t lens[],
		size_t argc);

	bool spop(const char* key, string& buf);
	int scard(const char* key);
	int smembers(const char* key, std::vector<string>& members);
	int smove(const char* src, const char* dst, const char* member);
	int smove(const char* src, const char* dst, const string& member);
	int smove(const char* src, const char* dst,
		const char* member, size_t len);

	int sinter(std::vector<string>& members, const char* first_key, ...);
	int sinter(const std::vector<const char*>& keys,
		std::vector<string>& members);
	int sinter(const std::vector<string>& keys,
		std::vector<string>& members);

	int sdiff(std::vector<string>& members, const char* first_key, ...);
	int sdiff(const std::vector<const char*>& keys,
		std::vector<string>& members);
	int sdiff(const std::vector<string>& keys,
		std::vector<string>& members);

	int sdiffstore(const char* dst, const char* first_key, ...);
	int sdiffstore(const char* dst, const std::vector<const char*>& keys);
	int sdiffstore(const char* dst, const std::vector<string>& keys);
};

} // namespace acl
