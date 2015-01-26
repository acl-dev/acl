#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

/**
 * redis Hahs(哈希表) 类，本类的实现的主要命令：
 * HDEL/HEXISTS/HGET/HGETALL/HINCRBY/HINCRBYFLOAT/HKEYS/HLEN/HMGET/HMSET
 * HSET/HSETNX/HVALS/HSCAN
 */
class ACL_CPP_API redis_hash : public redis_command
{
public:
	redis_hash(redis_client* conn = NULL);
	~redis_hash();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将多个"域-值"对添加至 KEY 对应的哈希表中
	 * @param key {const char*} 哈希表 key 值
	 * @return {bool} 添加是否成功
	 */
	bool hmset(const char* key, const std::map<string, string>& attrs);
	bool hmset(const char* key, const std::map<string, char*>& attrs);
	bool hmset(const char* key, const std::map<string, const char*>& attrs);

	bool hmset(const char* key, const std::map<int, string>& attrs);
	bool hmset(const char* key, const std::map<int, char*>& attrs);
	bool hmset(const char* key, const std::map<int, const char*>& attrs);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 根据 KEY 值将多个"域-值"对从哈希表中取出
	 * @param key {const char*} 哈希表 key 值
	 * @param names 对应 key 的域值对
	 * @param result {std::vector<string>*} 当该对象指针非空时存储查询结果
	 * @return {bool} 操作是否成功，如果返回成功，可以通过方法 hmget_result 根据
	 *  下标值取得对应的值，或直接调用 get_result 方法取得 redis_result，或者在
	 *  调用方法中传入非空的存储结果对象的地址
	 */
	bool hmget(const char* key, const std::vector<string>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const std::vector<char*>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const std::vector<const char*>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const std::vector<int>& names,
		std::vector<string>* result = NULL);

	bool hmget(const char* key, const char* names[], size_t argc,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const int names[], size_t argc,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const char* names[], const size_t lens[],
		size_t argc, std::vector<string>* result = NULL);

	/**
	 * 当 hmget 获得 true 时调用本方法来获得对应下标的值，下标顺序与 hmget
	 * 中的数组的下标顺序相同
	 * @param i {size_t} 下标（从 0 开始）
	 * @param len {size_t*} 若该指针非空，则存储所返回结果的长度（仅当该方法
	 *  返回非空指针时有效）
	 * @return {const char*} 返回对应下标的值，当返回 NULL 时表示该下标没有值，
	 *  为了保证使用上的安全性，返回的数据总能保证最后是以 \0 结尾，在计算数据长度
	 *  时不包含该结尾符
	 */
	const char* hmget_value(size_t i, size_t* len = NULL) const;

	/**
	 * 当查询结果为数组对象时调用本方法获得一个数组元素
	 * @param i {size_t} 数组对象的下标值
	 * @return {const redis_result*} 当结果非数组对象或结果为空或出错时该方法
	 *  返回 NULL
	 */
	const redis_result* hmget_child(size_t i) const;

	/**
	 * 返回查询结果集的个数
	 * @return {size_t}
	 */
	size_t hmget_size() const;

	/////////////////////////////////////////////////////////////////////

	/**
	 * 设置 key 对象中某个域字段的值
	 * @param key {const char*} key 键值
	 * @param name {const char*} key 对象的域名称
	 * @param value {const char*} key 对象的域值
	 * @return {int} 返回值含义：
	 *  1 -- 表示新添加的域字段添加成功
	 *  0 -- 表示更新已经存在的域字段成功
	 * -1 -- 表示出错或该 key 对象非哈希对象或从结点禁止修改
	 */
	int hset(const char* key, const char* name, const char* value);
	int hset(const char* key, const char* name,
		const char* value, size_t value_len);
	int hset(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);

	/**
	 * 当且仅当 key 对象中的某个域字段不存在时才更新该域字段值
	 * @param key {const char*} key 键值
	 * @param name {const char*} key 对象的域名称
	 * @param value {const char*} key 对象的域值
	 * @return {int} 返回值含义：
	 *  1 -- 表示新添加的域字段添加成功
	 *  0 -- 该域字段存在且未对其进行更新
	 * -1 -- 表示出错或该 key 对象非哈希对象或从结点禁止修改
	 */
	int hsetnx(const char* key, const char* name, const char* value);
	int hsetnx(const char* key, const char* name,
		const char* value, size_t value_len);
	int hsetnx(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);

	/**
	 * 从 redis 哈希表中获取某个 key 对象的某个域的值
	 * @param key {const char*} key 键值
	 * @param name {const char*} key 对象的域字段名称
	 * @param result {string&} 存储查询结果值(内部对该 string 进行内容追加)
	 * @return {bool} 返回值含义：
	 *  true -- 操作成功，当该域不存在时也返回成功，需要检查 result 内容是否变化，
	 *          比如可以通过检查 result.length() 的变化来表明是否查询到结果
	 *  false -- 操作失败或该 key 对象非哈希对象
	 */
	bool hget(const char* key, const char* name, string& result);
	bool hget(const char* key, const char* name,
		size_t name_len, string& result);

	/**
	 * 从 redis 哈希表中获取某个 key 对象的所有域字段的值
	 * @param key {const char*} key 键值
	 * @param result {std::map<string, string>&} 存储域字段名-值查询结果集
	 * @return {bool} 操作是否成功，含义：
	 *  true -- 操作成功，当该域不存在时也返回成功，需要检查 result 内容是否变化，
	 *          比如可以通过检查 result.size() 的变化来表明是否查询到结果
	 *  false -- 操作失败或该 key 对象非哈希对象
	 */
	bool hgetall(const char* key, std::map<string, string>& result);
	bool hgetall(const char* key, std::vector<string>& names,
		std::vector<string>& values);
	bool hgetall(const char* key, std::vector<const char*>& names,
		std::vector<const char*>& values);

	/**
	 * 从 redis 哈希表中删除某个 key 对象的某些域字段
	 * @param key {const char*} key 键值
	 * @param first_name {const char*} 第一个域字段名，最后一个字段必须是 NULL
	 * @return {int} 成功删除的域字段个数，返回 -1 表示出错或该 key 对象非哈希对象
	 */
	int hdel(const char* key, const char* first_name, ...)
		ACL_CPP_PRINTF(3, 4);;
	int hdel(const char* key, const char* names[], size_t argc);
	int hdel(const char* key, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel(const char* key, const std::vector<string>& names);
	int hdel(const char* key, const std::vector<char*>& names);
	int hdel(const char* key, const std::vector<const char*>& names);

	/**
	 * 当某个 key 对象中的某个域字段为整数时，对其进行加减操作
	 * @param key {const char*} key 键值
	 * @param name {const char*} key 对象的域字段名称
	 * @param inc {long long int} 增加的值，可以为负值
	 * @param result {long long int*} 非 NULL 时存储结果值
	 * @return {bool} 操作是否成功，当返回 false 时表明出错或该 key 对象非哈希
	 *  对象或该域字段非整数类型
	 */
	bool hincrby(const char* key, const char* name,
		long long int inc, long long int* result = NULL);

	/**
	 * 当某个 key 对象中的某个域字段为浮点数时，对其进行加减操作
	 * @param key {const char*} key 键值
	 * @param name {const char*} key 对象的域字段名称
	 * @param inc {double} 增加的值，可以为负值
	 * @param result {double*} 非 NULL 时存储结果值
	 * @return {bool} 操作是否成功，当返回 false 时表明出错或该 key 对象非哈希
	 *  对象或该域字段非浮点数类型
	 */
	bool hincrbyfloat(const char* key, const char* name,
		double inc, double* result = NULL);

	/**
	 * 返回 key 对象中所有域字段名称
	 * @param key {const char*} key 键值
	 * @param names {std::vector<string>&} 存储该 key 对象所有域字段名称
	 * @return {bool} 操作是否成功，返回 false 表明出错或该 key 对象非哈希对象
	 */
	bool hkeys(const char* key, std::vector<string>& names);

	/**
	 * 检查 key 对象中某个域字段是否存在
	 * @param key {const char*} key 键值
	 * @param name {const char*} key 对象的域字段名称
	 * @return {bool} 操作是否成功，返回 false 表明出错或该 key 对象非哈希对象
	 *  或该域字段不存在
	 */
	bool hexists(const char* key, const char* name);
	bool hexists(const char* key, const char* name, size_t name_len);

	/**
	 * 获得某个 key 对象中所有域字段的数量
	 * @param key {const char*} key 键值
	 * @return {int} 返回值含义：
	 *  -1 -- 出错或该 key 对象非哈希对象
	 *  >0 -- 域字段数量
	 *   0 -- 该 key 不存在或域字段数量为 0
	 */
	int hlen(const char* key);

	/**
	 * 命令用于迭代哈希键中的键值对
	 * @param key {const char*} 哈希键值
	 * @param cursor {int} 游标值，开始遍历时该值写 0
	 * @param out {std::map<string>&} 结果集
	 * @param pattern {const char*} 匹配模式，glob 风格，非空时有效
	 * @param count {const size_t*} 限定的结果集数量，非空指针时有效
	 * @return {int} 下一个游标位置，含义如下：
	 *   0：遍历结束
	 *  -1: 出错
	 *  >0: 游标的下一个位置
	 */
	int hscan(const char* key, int cursor, std::map<string, string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl
