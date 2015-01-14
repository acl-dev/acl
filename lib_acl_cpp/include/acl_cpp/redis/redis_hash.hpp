#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_hash
{
public:
	redis_hash(redis_client& conn);
	~redis_hash();

	const redis_result* get_result() const
	{
		return result_;
	}

	redis_client& get_client() const
	{
		return conn_;
	}

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将多个"域-值"对添加至 KEY 对应的哈希表中
	 * @param key {const char*} KEY 值
	 * @return {bool} 添加是否成功
	 */
	bool hmset(const char* key, const std::map<string, string>& attrs);
	bool hmset(const char* key, const std::map<string, char*>& attrs);
	bool hmset(const char* key, const std::map<string, const char*>& attrs);

	bool hmset(const char* key, const std::map<int, string>& attrs);
	bool hmset(const char* key, const std::map<int, char*>& attrs);
	bool hmset(const char* key, const std::map<int, const char*>& attrs);
	bool hmset(const string& req);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 根据 KEY 值将多个"域-值"对从哈希表中取出
	 * @param key {const char*} KEY 值
	 * @return {bool} 操作是否成功，如果返回成功，可以通过方法 hmget_result 根据
	 *  下标值取得对应的值，或直接调用 get_result 方法取得 redis_result
	 */
	bool hmget(const char* key, const std::vector<string>& names);
	bool hmget(const char* key, const std::vector<char*>& names);
	bool hmget(const char* key, const std::vector<const char*>& names);
	bool hmget(const char* key, const std::vector<int>& names);

	bool hmget(const char* key, const char* names[], size_t argc);
	bool hmget(const char* key, const int names[], size_t argc);
	bool hmget(const char* key, const char* names[],
		const size_t lens[], size_t argc);
	bool hmget(const string& req);

	/**
	 * 当 hmget 获得 true 时调用本方法来获得对应下标的值，下标顺序与 hmget 中的数组
	 * 的下标顺序相同
	 * @param i {size_t} 下标（从 0 开始）
	 * @param len {size_t*} 若该指针非空，则存储所返回结果的长度（仅当该方法返回非
	 *  空指针时有效）
	 * @return {const char*} 返回对应下标的值，当返回 NULL 时表示该下标没有值，
	 *  为了保证使用上的安全性，返回的数据总能保证最后是以 \0 结尾，在计算数据长度时
	 *  不包含该结尾符
	 */
	const char* hmget_result(size_t i, size_t* len = NULL) const;

	/////////////////////////////////////////////////////////////////////

	int hset(const char* key, const char* name, const char* value);
	int hset(const char* key, const char* name,
		const char* value, size_t value_len);
	int hset(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);
	int hset(const string& req);

	int hsetnx(const char* key, const char* name, const char* value);
	int hsetnx(const char* key, const char* name,
		const char* value, size_t value_len);
	int hsetnx(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);
	int hsetnx(const string& req);

	bool hget(const char* key, const char* name, string& result);
	bool hget(const char* key, const char* name,
		size_t name_len, string& result);

	bool hgetall(const char* key, std::map<string, string>& result);
	bool hgetall(const char* key, std::vector<string>& names,
		std::vector<string>& values);
	bool hgetall(const char* key, std::vector<const char*>& names,
		std::vector<const char*>& values);

	int hdel(const char* key, const char* first_name, ...)
		ACL_CPP_PRINTF(3, 4);;
	int hdel(const char* key, const char* names[], size_t argc);
	int hdel(const char* key, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel(const char* key, const std::vector<string>& names);
	int hdel(const char* key, const std::vector<char*>& names);
	int hdel(const char* key, const std::vector<const char*>& names);
	int hdel(const string& req);

	bool hincrby(const char* key, const char* name,
		long long int inc, long long int* result = NULL);
	bool hincrbyfloat(const char* key, const char* name,
		double inc, double* result = NULL);

	bool hkeys(const char* key, std::vector<string>& names);

	bool hexists(const char* key, const char* name);
	bool hexists(const char* key, const char* name, size_t name_len);

	int hlen(const char* key);

	/////////////////////////////////////////////////////////////////////

private:
	redis_client& conn_;
	const redis_result* result_;
};

} // namespace acl
