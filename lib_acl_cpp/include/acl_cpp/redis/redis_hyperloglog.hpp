#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_hyperloglog : public redis_command
{
public:
	redis_hyperloglog(redis_client* conn = NULL);
	~redis_hyperloglog();

	int pfadd(const char* key, const char* first_element, ...);
	int pfadd(const char* key, const std::vector<const char*>& elements);
	int pfadd(const char* key, const std::vector<string>& elements);

	int pfcount(const char* first_key, ...);
	int pfcount(const std::vector<const char*>& keys);
	int pfcount(const std::vector<string>& keys);

	bool pfmerge(const char* dst, const char* first_src, ...);
	bool pfmerge(const char* dst, const std::vector<const char*>& keys);
	bool pfmerge(const char* dst, const std::vector<string>& keys);
};

} // namespace acl
