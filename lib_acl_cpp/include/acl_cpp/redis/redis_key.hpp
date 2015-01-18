#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"

namespace acl {

class redis_result;

// redis 服务支持的数据类型分类
typedef enum
{
	REDIS_KEY_UNKNOWN,
	REDIS_KEY_NONE,
	REDIS_KEY_STRING,	// string
	REDIS_KEY_HASH,		// hash
	REDIS_KEY_LIST,		// list
	REDIS_KEY_SET,		// set
	REDIS_KEY_ZSET		// sorted set
} redis_key_t;

class ACL_CPP_API redis_key
{
public:
	redis_key(redis_client* conn = NULL);
	~redis_key();

	void reset();

	void set_client(redis_client* conn);

	redis_client* get_client() const
	{
		return conn_;
	}

	/**
	 * 删除一组 KEY
	 * @return {int} 返回所删除的 KEY 的个数，如下：
	 *  0: 未删除任何 KEY
	 *  -1: 出错
	 *  >0: 真正删除的 KEY 的个数，该值有可能少于输入的 KEY 的个数
	 */
	int del(const char* first_key, ...) ACL_CPP_PRINTF(2, 3);
	int del(const std::vector<string>& keys);
	int del(const std::vector<char*>& keys);
	int del(const std::vector<const char*>& keys);
	int del(const std::vector<int>& keys);
	int del(const char* keys[], size_t argc);
	int del(const int keys[], size_t argc);
	int del(const char* keys[], const size_t lens[], size_t argc);

	/**
	 * 设置 KEY 的生存周期，单位（秒）
	 * @param key {const char*} 键值
	 * @param n {int} 生存周期（秒）
	 * @return {int} 返回值含义如下：
	 *  > 0: 成功设置了生存周期
	 *  0：该 key 不存在或该键未设置生存周期
	 *  < 0: 出错
	 */
	int expire(const char* key, int n);

	/**
	 * 获得 KEY 的剩余生存周期，单位（秒）
	 * @param key {const char*} 键值
	 * @return {int} 返回对应键值的生存周期
	 *  > 0: 该 key 剩余的生存周期（秒）
	 *  0：该 key 不存在或该键未设置生存周期
	 *  < 0: 出错
	 */
	int ttl(const char* key);

	/**
	 * 判断 KEY 是否存在
	 * @param key {const char*} KEY 值
	 * @return {bool} 返回 true 表示存在，否则表示出错或不存在
	 */
	bool exists(const char* key);

	/**
	 * 获得 KEY 的存储类型
	 * @para key {const char*} KEY 值
	 * @return {redis_key_t} 返回 KEY 的存储类型
	 */
	redis_key_t type(const char* key);

	bool migrate(const char* key, const char* addr, unsigned dest_db,
		unsigned timeout, const char* option = NULL);

	int move(const char* key, unsigned dest_db);

private:
	redis_client* conn_;
	const redis_result* result_;

	int del(const string& req);
};

} // namespace acl
