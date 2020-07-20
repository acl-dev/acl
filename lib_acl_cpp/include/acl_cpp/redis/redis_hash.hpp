#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "../stdlib/string.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;

/**
 * redis Hash(哈希表) 类，本类的实现的主要命令：
 * redis Hash class, include commands as below:
 * HDEL/HEXISTS/HGET/HGETALL/HINCRBY/HINCRBYFLOAT/HKEYS/HLEN/HMGET/HMSET
 * HSET/HSETNX/HVALS/HSCAN
 */
class ACL_CPP_API redis_hash : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_hash(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_hash(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_hash(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_hash(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_hash(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将多个"域-值"对添加至 KEY 对应的哈希表中
	 * HMSET: set the key's multiple fileds in redis-server
	 * @param key {const char*} 哈希表 key 值
	 *  the hash key for Hash class
	 * @param attrs {const std::map<acl::string, ...>&} the fileds in map
	 * @return {bool} 添加是否成功
	 *  if successful for HMSET command
	 */
	bool hmset(const char* key, const std::map<string, string>& attrs);
	bool hmset(const char* key, size_t klen,
		const std::map<string, string>& attrs);
	bool hmset(const char* key, const std::map<string, const char*>& attrs);
	bool hmset(const char* key, const std::vector<string>& names,
		const std::vector<string>& values);
	bool hmset(const char* key, size_t klen,
		const std::vector<string>& names,
		const std::vector<string>& values);
	bool hmset(const char* key, const std::vector<const char*>& names,
		const std::vector<const char*>& values);
	bool hmset(const char* key, const char* names[], const char* values[],
		size_t argc);
	bool hmset(const char* key, const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);
	bool hmset(const char* key, size_t klen, const char* names[],
		const size_t names_len[], const char* values[],
		const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 根据 KEY 值将多个"域-值"对从哈希表中取出
	 * get the values associated with the specified fields
	 * in the hash stored at key
	 * @param key {const char*} 哈希表 key 值
	 *  the hash key
	 * @param names 对应 key 的域值对
	 *  the given hash fileds
	 * @param result {std::vector<acl::string>*} 当该对象指针非空时存储查询结果；
	 *  如果该参数为 NULL 时，则可以通过基类 result_/get_ 获得数据
	 *  store the result of the given hash files if not NULL.
	 *  If NULL, the base class's method like result_/get can be used
	 *  to get the values
	 * @return {bool} 操作是否成功，操作成功后可以通过以下任一种方式获得数据：
	 *  if successul, one of below ways can be used to get the result:
	 *
	 *  1、在调用方法中传入非空的存储结果对象的地址
	 *     input the no-NULL result parameter when call hmget, when
	 *     success, the result will store the values of the given fileds
	 *
	 *  2、基类方法 result_value 获得指定下标的元素数据
	 *     call redis_command::result_value with the specified subscript
	 *
	 *  3、基类方法 result_child 获得指定下标的元素对象(redis_result），然后再通过
	 *     redis_result::argv_to_string 方法获得元素数据
	 *     call redis_command::result_child with specified subscript to
	 *     get redis_result object, then call redis_result::argv_to_string
	 *     with above result to get the values of the give fileds
	 *
	 *  4、基类方法 get_result 方法取得总结果集对象 redis_result，然后再通过
	 *     redis_result::get_child 获得一个元素对象，然后再通过方式 2 中指定
	 *     的方法获得该元素的数据
	 *     call redis_command::get_result with the specified subscript to
	 *     get redis_result object, and use redis_result::get_child to
	 *     get one result object, then call redis_result::argv_to_string
	 *     to get the value of one filed.
	 *
	 *  5、基类方法 get_children 获得结果元素数组对象，再通过 redis_result 中
	 *     的方法 argv_to_string 从每一个元素对象中获得元素数据
	 *     use redis_command::get_children to get the redis_result array,
	 *     then use redis_result::argv_to_string to get every value of
	 *     the given fileds
	 */
	bool hmget(const char* key, const std::vector<string>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, size_t klen,
		const std::vector<string>& names,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const std::vector<const char*>& names,
		std::vector<string>* result = NULL);

	bool hmget(const char* key, const char* names[], size_t argc,
		std::vector<string>* result = NULL);
	bool hmget(const char* key, const char* names[], const size_t lens[],
		size_t argc, std::vector<string>* result = NULL);
	bool hmget(const char* key, size_t klen,
		const char* names[], const size_t lens[],
		size_t argc, std::vector<string>* result = NULL);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 设置 key 对象中某个域字段的值
	 * set one field's value in the hash stored at key.
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域名称
	 *  the filed name of the hash key
	 * @param value {const char*} key 对象的域值
	 *  the filed value of the hash key
	 * @return {int} 返回值含义：
	 *  1 -- 表示新添加的域字段添加成功
	 *  0 -- 表示更新已经存在的域字段成功
	 * -1 -- 表示出错或该 key 对象非哈希对象或从结点禁止修改
	 *  return int value as below:
	 *  1 -- this is a new filed and set ok
	 *  0 -- thie is a old filed and set ok
	 * -1 -- error happend or the key is not a Hash type
	 */
	int hset(const char* key, const char* name, const char* value);
	int hset(const char* key, const char* name,
		const char* value, size_t value_len);
	int hset(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);
	int hset(const char* key, size_t klen, const char* name,
		size_t name_len, const char* value, size_t value_len);

	/**
	 * 当且仅当 key 对象中的某个域字段不存在时才更新该域字段值
	 * set one new field of one key in hash only when the filed isn't
	 * existing.
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域名称
	 *  the field name
	 * @param value {const char*} key 对象的域值
	 *　the field value
	 * @return {int} 返回值含义：
	 *  1 -- 表示新添加的域字段添加成功
	 *  0 -- 该域字段存在且未对其进行更新
	 * -1 -- 表示出错或该 key 对象非哈希对象或从结点禁止修改
	 *
	 *  return int value as below:
	 *  1 -- this is a new filed and set ok
	 *  0 -- thie is a old filed and not set
	 * -1 -- error happend or the key is not a Hash type
	 */
	int hsetnx(const char* key, const char* name, const char* value);
	int hsetnx(const char* key, const char* name,
		const char* value, size_t value_len);
	int hsetnx(const char* key, const char* name, size_t name_len,
		const char* value, size_t value_len);
	int hsetnx(const char* key, size_t klen, const char* name,
		size_t name_len, const char* value, size_t value_len);

	/**
	 * 从 redis 哈希表中获取某个 key 对象的某个域的值
	 * get the value assosiated with field in the hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域字段名称
	 *  the field's name
	 * @param result {acl::string&} 存储查询结果值(内部对该 string 进行内容追加)
	 *  store the value result of the given field
	 * @return {bool} 返回值含义：
	 *  true -- 操作成功，当result为空时表示 KEY 或字段域不存在
	 *          get the value associated with field; if result is empty then
	 *          the key or the name field doesn't exist
	 *  false -- 域字段不存在或操作失败或该 key 对象非哈希对象
	 *           the field not exists, or error happened,
	 *           or the key isn't a hash key
	 */
	bool hget(const char* key, const char* name, string& result);
	bool hget(const char* key, const char* name,
		size_t name_len, string& result);
	bool hget(const char* key, size_t klen, const char* name,
		size_t name_len, string& result);

	/**
	 * 从 redis 哈希表中获取某个 key 对象的所有域字段的值
	 * get all the fields and values in hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param result {std::map<string, string>&} 存储域字段名-值查询结果集
	 *  store the result of all the fileds and values
	 * @return {bool} 操作是否成功，含义：
	 *  if ok, show below:
	 *  true -- 操作成功，当该域不存在时也返回成功，需要检查 result 内容是否变化，
	 *          比如可以通过检查 result.size() 的变化来表明是否查询到结果
	 *          successful if the key is a hash key or the key not exists
	 *  false -- 操作失败或该 key 对象非哈希对象
	 *           error happened or the key isn't a hash key
	 */
	bool hgetall(const char* key, std::map<string, string>& result);
	bool hgetall(const char* key, size_t klen,
		std::map<string, string>& result);
	bool hgetall(const char* key, std::vector<string>& names,
		std::vector<string>& values);
	bool hgetall(const char* key, size_t klen,
		std::vector<string>& names, std::vector<string>& values);
	bool hgetall(const char* key, std::vector<const char*>& names,
		std::vector<const char*>& values);

	/**
	 * 从 redis 哈希表中删除某个 key 对象的某些域字段
	 * remove one or more fields from hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param first_name {const char*} 第一个域字段名，最后一个字段必须是 NULL
	 *  the first field of the fields list, the last field must be NULL
	 *  indicating the end of vary parameters
	 * @return {int} 成功删除的域字段个数，返回 -1 表示出错或该 key 对象非哈希对象
	 *  return the number of fields be removed successfully, or -1 when
	 *  error happened or operating on a no hash key
	 */
	int hdel(const char* key, const char* first_name);
	int hdel(const char* key, const char* names[], size_t argc);
	int hdel(const char* key, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel(const char* key, size_t klen, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel(const char* key, const std::vector<string>& names);
	int hdel(const char* key, size_t klen, const std::vector<string>& names);
	int hdel(const char* key, const std::vector<const char*>& names);
	int hdel_fields(const char* key, const char* names[], size_t argc);
	int hdel_fields(const char* key, const char* names[],
		const size_t names_len[], size_t argc);
	int hdel_fields(const char* key, size_t klen,
		const char* names[], const size_t names_len[], size_t argc);
	int hdel_fields(const char* key, const std::vector<string>& names);
	int hdel_fields(const char* key, size_t klen,
		const std::vector<string>& names);
	int hdel_fields(const char* key, const std::vector<const char*>& names);
	int hdel_fields(const char* key, const char* first_name, ...);

	/**
	 * 当某个 key 对象中的某个域字段为整数时，对其进行加减操作
	 * inc(+n) or dec(-n) on a integer filed in hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域字段名称
	 *  the filed name of integer type
	 * @param inc {long long int} 增加的值，可以为负值
	 *  the integer value to be inc or dec on the field's value
	 * @param result {long long int*} 非 NULL 时存储结果值
	 *  store the result if non-NULL
	 * @return {bool} 操作是否成功，当返回 false 时表明出错或该 key 对象非哈希
	 *  对象或该域字段非整数类型
	 *  if successful: false when error, not a hash, or the field isn't
	 *  integer type
	 */
	bool hincrby(const char* key, const char* name,
		long long int inc, long long int* result = NULL);

	/**
	 * 当某个 key 对象中的某个域字段为浮点数时，对其进行加减操作
	 * inc(+n) or dec(-n) on a float filed in hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域字段名称
	 *  the filed name of float type
	 * @param inc {double} 增加的值，可以为负值
	 *  the float value to be inc or dec on the field's value
	 * @param result {double*} 非 NULL 时存储结果值
	 *  store the result if non-NULL
	 * @return {bool} 操作是否成功，当返回 false 时表明出错或该 key 对象非哈希
	 *  对象或该域字段非浮点数类型
	 *  if successful: false when error, not a hash, or the field isn't
	 *  float type
	 */
	bool hincrbyfloat(const char* key, const char* name,
		double inc, double* result = NULL);

	/**
	 * 返回 key 对象中所有域字段名称
	 * get all the fields in hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param names {std::vector<string>&} 存储该 key 对象所有域字段名称
	 *  store all the names of all fileds
	 * @return {bool} 操作是否成功，返回 false 表明出错或该 key 对象非哈希对象
	 *  return true on success, false if error happened or the
	 *  key wasn't a hash key
	 */
	bool hkeys(const char* key, std::vector<string>& names);
	bool hkeys(const char* key, size_t klen, std::vector<string>& names);

	/**
	 * 检查 key 对象中某个域字段是否存在
	 * check if the field exists in hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域字段名称
	 *  the filed's name of the key
	 * @return {bool} 操作是否成功，返回 false 表明出错或该 key 对象非哈希对象
	 *  或该域字段不存在
	 *  return true on success, false if error happened or the
	 *  key wasn't a hash key
	 */
	bool hexists(const char* key, const char* name);
	bool hexists(const char* key, const char* name, size_t name_len);
	bool hexists(const char* key, size_t klen, const char* name, size_t name_len);

	/**
	 * 获得指定 key 的所有字段值
	 * get all fields' values with the specified key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param values {std::vector<string>&} 存储结果
	 *  store the results
	 * @return {bool} 操作是否成功
	 *  return true on success, or failed when error happened
	 */
	bool hvals(const char* key, std::vector<string>& values);
	bool hvals(const char* key, size_t klen, std::vector<string>& values);

	/**
	 * 获得某个 key 对象中所有域字段的数量
	 * get the count of fields in hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @return {int} 返回值含义：
	 *  return int value as below:
	 *  -1 -- 出错或该 key 对象非哈希对象
	 *        error or not a hash key
	 *  >0 -- 域字段数量
	 *        the count of fields
	 *   0 -- 该 key 不存在或域字段数量为 0
	 *        key not exists or no fields in hash stored at key 
	 */
	int hlen(const char* key);
	int hlen(const char* key, size_t klen);

	/**
	 * 获得某个 key 中的指定域的数据长度
	 * Returns the string length of the value associated with field
	 * in the hash stored at key
	 * @param key {const char*} key 键值
	 *  the hash key
	 * @param name {const char*} key 对象的域字段名称
	 *  the field's name
	 * @return {int} 如果 key 或 name 不存在，则返回 0，如果 key 非哈希
	 *  键或出错，则返回 -1
	 *  If the key or the field do not exist, 0 is returned; If the key is
	 *  not the hash key or error happened, -1 is returned.
	 */
	int hstrlen(const char* key, const char* name, size_t name_len);
	int hstrlen(const char* key, size_t klen, const char* name, size_t name_len);
	int hstrlen(const char* key, const char *name);
	
	/**
	 * 命令用于迭代哈希键中的键值对
	 * scan the name and value of all fields in hash stored at key
	 * @param key {const char*} 哈希键值
	 *  the hash key
	 * @param cursor {int} 游标值，开始遍历时该值写 0
	 *  the cursor value, which is 0 at begin
	 * @param out {std::map<acl::string>&} 存储结果集，内部以追加方式将本次
	 *  遍历结果添加进该对象中，为防止因总结果集过大导致该数组溢出，用户可在
	 *  调用本函数前后清理该对象
	 *  store scaning result in appending mode
	 * @param pattern {const char*} 匹配模式，glob 风格，非空时有效
	 *  match pattern, effective only on no-NULL
	 * @param count {const size_t*} 限定的结果集数量，非空指针时有效
	 *  the max count of one scan process, effective only on no-NULL
	 * @return {int} 下一个游标位置，含义如下：
	 *  return the next cursor position, as below:
	 *   0：遍历结束
	 *     scan finish
	 *  -1: 出错
	 *     some error happened
	 *  >0: 游标的下一个位置，即使这样，具体有多少结果还需要检查 out，因为有可能为空
	 *     the next cursor postion to scan
	 */
	int hscan(const char* key, int cursor, std::map<string, string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
	int hscan(const char* key, size_t klen, int cursor,
		std::map<string, string>& out, const char* pattern = NULL,
		const size_t* count = NULL);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
