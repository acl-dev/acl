#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_list : public redis_command
{
public:
	redis_list(redis_client* conn = NULL);
	~redis_list();

	/////////////////////////////////////////////////////////////////////

	int llen(const char* key);
	bool lindex(const char* key, size_t idx, string& buf,
		bool* exist = NULL);
	bool lset(const char* key, size_t idx, const char* value);
	bool lset(const char* key, size_t idx, const char* value, size_t len);

	int linsert_before(const char* key, const char* pivot,
		const char* value);
	int linsert_before(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);
	int linsert_after(const char* key, const char* pivot,
		const char* value);
	int linsert_after(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	int lpush(const char* key, const char* first_value, ...);
	int lpush(const char* key, const char* values[], size_t argc);
	int lpush(const char* key, const std::vector<string>& values);
	int lpush(const char* key, const std::vector<char*>& values);
	int lpush(const char* key, const std::vector<const char*>& values);
	int lpush(const char* key, const char* values[], size_t lens[],
		size_t argc);

	int rpush(const char* key, const char* first_value, ...);
	int rpush(const char* key, const char* values[], size_t argc);
	int rpush(const char* key, const std::vector<string>& values);
	int rpush(const char* key, const std::vector<char*>& values);
	int rpush(const char* key, const std::vector<const char*>& values);
	int rpush(const char* key, const char* values[], size_t lens[],
		size_t argc);

	int lpushx(const char* key, const char* value);
	int lpushx(const char* key, const char* value, size_t len);
	int rpushx(const char* key, const char* value);
	int rpushx(const char* key, const char* value, size_t len);

	int lpop(const char* key, string& buf);
	int rpop(const char* key, string& buf);

	bool blpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool blpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool blpop(const std::vector<char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool blpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	bool brpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool brpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool brpop(const std::vector<char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool brpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	bool rpoplpush(const char* src, const char* dst, string* buf = NULL);
	bool brpoplpush(const char* src, const char* dst, size_t timeout,
		string* buf = NULL);

	bool lrange(const char* key, size_t start, size_t end,
		std::vector<string>& result);

	int lrem(const char* key, int count, const char* value);
	int lrem(const char* key, int count, const char* value, size_t len);

	bool ltrim(const char* key, size_t start, size_t end);

private:
	int linsert(const char* key, const char* pos, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);
	int pushx(const char* cmd, const char* key,
		const char* value, size_t len);
	int pop(const char* cmd, const char* key, string& buf);
	bool bpop(const char* cmd, const std::vector<const char*>& keys,
		size_t timeout, std::pair<string, string>& result);
	bool bpop(const char* cmd, const std::vector<string>& keys,
		size_t timeout, std::pair<string, string>& result);
	bool bpop(const string& req, std::pair<string, string>& result);
};

} // namespace acl
