#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class string;
class redis_client;
class redis_result;

class ACL_CPP_API redis_string : public redis_command
{
public:
	redis_string(redis_client* conn = NULL);
	~redis_string();

	/////////////////////////////////////////////////////////////////////

	bool set(const char* key, const char* value);
	bool set(const char* key, size_t key_len,
		const char* value, size_t value_len);

	bool setex(const char* key, const char* value, int timeout);
	bool setex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	int setnx(const char* key, const char* value);
	int setnx(const char* key, size_t key_len,
		const char* value, size_t value_len);

	int append(const char* key, const char* value);
	int append(const char* key, const char* value, size_t size);

	bool get(const char* key, string& buf);
	bool get(const char* key, size_t len, string& buf);
	const redis_result* get(const char* key);
	const redis_result* get(const char* key, size_t len);

	bool getset(const char* key, const char* value, string& buf);
	bool getset(const char* key, size_t key_len,
		const char* value, size_t value_len, string& buf);

	/////////////////////////////////////////////////////////////////////

	int str_len(const char* key);
	int str_len(const char* key, size_t len);

	int setrange(const char* key, unsigned offset, const char* value);
	int setrange(const char* key, size_t key_len, unsigned offset,
		const char* value, size_t value_len);

	bool getrange(const char* key, int start, int end, string& buf);
	bool getrange(const char* key, size_t key_len,
		int start, int end, string& buf);

	/////////////////////////////////////////////////////////////////////

	bool setbit(const char* key, unsigned offset, int bit);
	bool setbit(const char* key, size_t len, unsigned offset, int bit);

	bool getbit(const char* key, unsigned offset, int& bit);
	bool getbit(const char* key, size_t len, unsigned offset, int& bit);

	int bitcount(const char* key);
	int bitcount(const char* key, size_t len);
	int bitcount(const char* key, int start, int end);
	int bitcount(const char* key, size_t len, int start, int end);

	int bitop_and(const char* destkey, const std::vector<string>& keys);
	int bitop_or(const char* destkey, const std::vector<string>& keys);
	int bitop_xor(const char* destkey, const std::vector<string>& keys);

	int bitop_and(const char* destkey, const std::vector<const char*>& keys);
	int bitop_or(const char* destkey, const std::vector<const char*>& keys);
	int bitop_xor(const char* destkey, const std::vector<const char*>& keys);

	int bitop_and(const char* destkey, const char* key, ...);
	int bitop_or(const char* destkey, const char* key, ...);
	int bitop_xor(const char* destkey, const char* key, ...);

	int bitop_and(const char* destkey, const char* keys[], size_t size);
	int bitop_or(const char* destkey, const char* keys[], size_t size);
	int bitop_xor(const char* destkey, const char* keys[], size_t size);

	/////////////////////////////////////////////////////////////////////

	bool mset(const std::map<string, string>& objs);
	bool mset(const std::map<int, string>& objs);

	bool mset(const std::vector<string>& keys,
		const std::vector<string>& values);
	bool mset(const std::vector<int>& keys,
		const std::vector<string>& values);

	bool mset(const char* keys[], const char* values[], size_t argc);
	bool mset(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	int msetnx(const std::map<string, string>& objs);
	int msetnx(const std::map<int, string>& objs);

	int msetnx(const std::vector<string>& keys,
		const std::vector<string>& values);
	int msetnx(const std::vector<int>& keys,
		const std::vector<string>& values);

	int msetnx(const char* keys[], const char* values[], size_t argc);
	int msetnx(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	bool mget(const std::vector<string>& keys,
		std::vector<string>* out = NULL);
	bool mget(const std::vector<const char*>& keys,
		std::vector<string>* out = NULL);
	bool mget(const std::vector<int>& keys,
		std::vector<string>* out = NULL);

	bool mget(std::vector<string>* result, const char* first_key, ...)
		ACL_CPP_PRINTF(3, 4);;
	bool mget(const char* keys[], size_t argc,
		std::vector<string>* out = NULL);
	bool mget(const int keys[], size_t argc,
		std::vector<string>* out = NULL);
	bool mget(const char* keys[], const size_t keys_len[], size_t argc,
		std::vector<string>* out = NULL);

	size_t mget_size() const;
	const char* mget_value(size_t i, size_t* len = NULL) const;
	const redis_result* mget_child(size_t i) const;

	/////////////////////////////////////////////////////////////////////

	bool incr(const char* key, long long int* result = NULL);
	bool incrby(const char* key, long long int inc,
		long long int* result = NULL);
	bool incrbyfloat(const char* key, double inc, double* result = NULL);

	bool decr(const char* key, long long int* result = NULL);
	bool decrby(const char* key, long long int dec,
		long long int* result = NULL);

	bool incoper(const char* cmd, const char* key, long long int inc,
		long long int* result);

	/////////////////////////////////////////////////////////////////////

private:
	int bitop(const char* op, const char* destkey,
		const std::vector<string>& keys);
	int bitop(const char* op, const char* destkey,
		const std::vector<const char*>& keys);
	int bitop(const char* op, const char* destkey,
		const char* keys[], size_t size);
};

} // namespace acl
