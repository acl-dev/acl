#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl {

class redis_client;
class redis_client_cluster;

// redis 服务支持的数据类型分类
// the data type supported by redis
typedef enum
{
	REDIS_KEY_NONE,		// none
	REDIS_KEY_STRING,	// string
	REDIS_KEY_HASH,		// hash
	REDIS_KEY_LIST,		// list
	REDIS_KEY_SET,		// set
	REDIS_KEY_ZSET		// sorted set
} redis_key_t;

class ACL_CPP_API redis_key : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_key(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_key(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*, size_t)
	 */
	redis_key(redis_client_cluster* cluster, size_t max_conns = 0);

	virtual ~redis_key(void);

	/**
	 * 删除一个或一组 KEY，对于变参的接口，则要求最后一个参数必须以 NULL 结束
	 * delete one or some keys from redis, for deleting a variable
	 * number of keys, the last key must be NULL indicating the end
	 * of the variable args
	 * @return {int} 返回所删除的 KEY 的个数，如下：
	 *  0: 未删除任何 KEY
	 *  -1: 出错
	 *  >0: 真正删除的 KEY 的个数，该值有可能少于输入的 KEY 的个数
	 *  return the number of keys been deleted, return value as below:
	 *  0: none key be deleted
	 * -1: error happened
	 *  >0: the number of keys been deleted
	 *
	 */
	int del_one(const char* key);
	int del_one(const char* key, size_t len);
	int del(const char* key);
	int del(const std::vector<string>& keys);
	int del(const std::vector<const char*>& keys);
	int del(const char* keys[], size_t argc);
	int del(const char* keys[], const size_t lens[], size_t argc);
	int del_keys(const char* first_key, ...);
	int del_keys(const std::vector<string>& keys);
	int del_keys(const std::vector<const char*>& keys);
	int del_keys(const char* keys[], size_t argc);
	int del_keys(const char* keys[], const size_t lens[], size_t argc);

	/**
	 * 序列化给定 key ，并返回被序列化的值，使用 RESTORE 命令可以将这个值反序列化
	 * 为 Redis 键
	 * serialize the object associate with the given key, and get the
	 * value after serializing, RESTORE command can be used to
	 * deserialize by the value
	 * @param key {const char*} 键值
	 *  the key
	 * @param out {string&} 存储序列化的二进制数据
	 *  buffur used to store the result
	 * @return {int} 序列化后数据长度
	 *  the length of the data after serializing
	 */
	int dump(const char* key, string& out);

	/**
	 * 判断 KEY 是否存在
	 * check if the key exists in redis
	 * @param key {const char*} KEY 值
	 *  the key
	 * @return {bool} 返回 true 表示存在，否则表示出错或不存在
	 *  true returned if key existing, false if error or not existing
	 */
	bool exists(const char* key);

	/**
	 * 设置 KEY 的生存周期，单位（秒）
	 * set a key's time to live in seconds
	 * @param key {const char*} 键值
	 *  the key
	 * @param n {int} 生存周期（秒）
	 *  lief cycle in seconds
	 * @return {int} 返回值含义如下：
	 *  return value as below:
	 *  > 0: 成功设置了生存周期
	 *       set successfully
	 *  0：该 key 不存在
	 *    the key doesn't exist
	 *  < 0: 出错
	 *       error happened
	 */
	int expire(const char* key, int n);

	/**
	 * 用 UNIX 时间截设置 KEY 的生存周期
	 * set the expiration for a key as a UNIX timestamp
	 * @param key {const char*} 对象键值
	 *  the key
	 * @param stamp {time_t} UNIX 时间截，即自 1970 年以来的秒数
	 *  an absolute Unix timestamp (seconds since January 1, 1970).
	 * @return {int} 返回值的含义：
	 *  return value:
	 *  1: 设置成功
	 *     the timeout was set
	 *  0: 该 key 不存在
	 *     the key doesn't exist or the timeout couldn't be set
	 * -1: 出错
	 *     error happened
	 */
	int expireat(const char* key, time_t stamp);

	/**
	 * 查找所有符合给定模式 pattern 的 key
	 * find all keys matching the given pattern
	 * @param pattern {const char*} 匹配模式
	 *  the give matching pattern
	 * @param out {std::vector<string>*} 非 NULL 时用来存储结果集
	 *  store the matched keys
	 * @return {int} 结果集的数量，0--为空，<0 -- 表示出错
	 *  return the number of the matched keys, 0 if none, < 0 if error
	 *  匹配模式举例：
	 *   KEYS * 匹配数据库中所有 key 。
	 *   KEYS h?llo 匹配 hello ， hallo 和 hxllo 等。
	 *   KEYS h*llo 匹配 hllo 和 heeeeello 等。
	 *   KEYS h[ae]llo 匹配 hello 和 hallo ，但不匹配 hillo 。
	 *
	 *  操作成功后可以通过以下任一方式获得数据
	 *  1、基类方法 get_value 获得指定下标的元素数据
	 *  2、基类方法 get_child 获得指定下标的元素对象(redis_result），然后再通过
	 *     redis_result::argv_to_string 方法获得元素数据
	 *  3、基类方法 get_result 方法取得总结果集对象 redis_result，然后再通过
	 *     redis_result::get_child 获得一个元素对象，然后再通过方式 2 中指定
	 *     的方法获得该元素的数据
	 *  4、基类方法 get_children 获得结果元素数组对象，再通过 redis_result 中
	 *     的方法 argv_to_string 从每一个元素对象中获得元素数据
	 *  5、在调用方法中传入非空的存储结果对象的地址
	 */
	int keys_pattern(const char* pattern, std::vector<string>* out);
	
	/**
	 * 将数据从一个 redis-server 迁移至另一个 redis-server
	 * atomically transfer a key from a redis instance to another one
	 * @param key {const char*} 数据对应的键值
	 *  the key
	 * @param addr {const char*} 目标 redis-server 服务器地址，格式：ip:port
	 *  the destination redis instance's address, format: ip:port
	 * @param dest_db {unsigned} 目标 redis-server 服务器的数据库 ID 号
	 *  the databases ID in destination redis
	 * @param timeout {unsigned} 迁移过程的超时时间(毫秒级)
	 *  timeout(microseconds) in transfering
	 * @param option {const char*} COPY 或 REPLACE
	 *  transfer option: COPY or REPLACE
	 * @return {bool} 迁移是否成功
	 *  if transfering successfully
	 */
	bool migrate(const char* key, const char* addr, unsigned dest_db,
		unsigned timeout, const char* option = NULL);

	/**
	 * 将数据移至本 redis-server 中的另一个数据库中
	 * move a key to another database
	 * @param key {const char*} 数据键值
	 *  the key
	 * @param dest_db {unsigned} 目标数据库 ID 号
	 *  the destination database
	 * @return {int} 迁移是否成功。-1: 表示出错，0：迁移失败，因为目标数据库中存在
	 *  相同键值，1：迁移成功
	 *  if moving succcessfully. -1 if error, 0 if moving failed because
	 *  the same key already exists, 1 if successful
	 */
	int move(const char* key, unsigned dest_db);

	/**
	 * 返回给定 key 引用所储存的值的次数。此命令主要用于除错。
	 * get the referring count of the object, which just for debugging
	 * @param key {const char*} 数据键值
	 *  the key
	 * @return {int} 返回 0 表示该 key 不存在；< 0 表示出错
	 *  0 if key not exists, < 0 if error
	 */
	int object_refcount(const char* key);

	/**
	 * 返回给定 key 键储存的值所使用的内部表示
	 * get the internal storing of the object assosicate with the key
	 * @param key {const char*} 数据键值
	 *  the key
	 * @param out {string&} 存在结果
	 *  store the result
	 * @return {bool} 是否成功
	 *  if successful
	 */
	bool object_encoding(const char* key, string& out);

	/**
	 * 返回给定 key 自储存以来的空闲时间(idle， 没有被读取也没有被写入)，以秒为单位
	 * get the key's idle time in seconds since its first stored
	 * @param key {const char*} 数据键值
	 *  the key
	 * @return {int} 返回值 < 0 表示出错
	 *  0 if error happened
	 */
	int object_idletime(const char* key);

	/**
	 * 移除给定 key 的生存时间，将这个 key 从"易失的"(带生存时间 key )转换成
	 * "持久的"(一个不带生存时间、永不过期的 key )
	 * remove the expiration from a key
	 * @param key {const char*} 对象键值
	 *  the key
	 * @return {int} 返回值的含义如下：
	 *  the value returned as below:
	 *  1 -- 设置成功
	 *       set ok
	 *  0 -- 该 key 不存在或未设置过期时间
	 *       the key not exists or not be set expiration
	 * -1 -- 出错
	 *       error happened
	 */
	int persist(const char* key);

	/**
	 * 设置 KEY 的生存周期，单位（毫秒）
	 * set a key's time to live in milliseconds
	 * @param key {const char*} 键值
	 *  the key
	 * @param n {int} 生存周期（毫秒）
	 *  time to live in milliseconds
	 * @return {int} 返回值含义如下：
	 *  value returned as below:
	 *  > 0: 成功设置了生存周期
	 *       set successfully
	 *    0：该 key 不存在
	 *       the key doesn't exist
	 *  < 0: 出错
	 *       error happened
	 */
	int pexpire(const char* key, int n);

	/**
	 * 以毫秒为单位设置 key 的过期 unix 时间戳
	 * set the expiration for a key as UNIX timestamp specified
	 * in milliseconds
	 * @param key {const char*} 键值
	 *  the key
	 * @param n {long long int} UNIX 时间截，即自 1970 年以来的毫秒数
	 *  the UNIX timestamp in milliseconds from 1970.1.1
	 * @return {int} 返回值含义如下：
	 *  value resturned as below:
	 *  > 0: 成功设置了生存周期
	 *       set successfully
	 *    0：该 key 不存在
	 *       the key doesn't exist
	 *  < 0: 出错
	 *       error happened
	 */
	int pexpireat(const char* key, long long int n);

	/**
	 * 获得 KEY 的剩余生存周期，单位（毫秒）
	 * get the time to live for a key in milliseconds
	 * @param key {const char*} 键值
	 *  the key
	 * @return {int} 返回对应键值的生存周期
	 *  value returned as below:
	 *  >0: 该 key 剩余的生存周期（毫秒）
	 *      the time to live for a key in milliseconds
	 *  -3：出错
	 *      error happened
	 *  -2：key 不存在
	 *      the key doesn't exist
	 *  -1：当 key 存在但没有设置剩余时间
	 *      th key were not be set expiration
	 * 注：对于 redis-server 2.8 以前版本，key 不存在或存在但未设置生存期则返回 -1
	 * notice: for redis version before 2.8, -1 will be returned if the
	 * key doesn't exist or the key were not be set expiration.
	 */
	long long int pttl(const char* key);

	/**
	 * 从当前数据库中随机返回(不会删除)一个 key
	 * return a random key from the keyspace
	 ＊@param buf {string&} 成功获得随机 KEY 时存储结果
	 *  store the key
	 * @return {bool} 操作是否成功，当出错或 key 不存在时返回 false
	 *  true on success, or false be returned
	 */
	bool randmkey(string& buf);

	/**
	 * 将 key 改名为 newkey
	 * rename a key
	 * @return {bool}
	 *  true on success, or error happened
	 */
	bool rename_key(const char* key, const char* newkey);

	/**
	 * 当且仅当 newkey 不存在时，将 key 改名为 newkey
	 * rename a key only if the new key does not exist
	 * @param key {const char*} 旧 key
	 * @param newkey {const char*} 新 key
	 * @return {bool} 是否成功
	 *  true on success, false if the newkey already existed or error
	 */
	bool renamenx(const char* key, const char* newkey);

	/**
	 * 反序列化给定的序列化值，并将它和给定的 key 关联
	 * create a key using the provided serialized value, previously
	 * obtained by using DUMP
	 * @param ttl {int} 以毫秒为单位为 key 设置生存时间，如果 ttl 为 0，
	 *  那么不设置生存时间
	 *  the time to live for the key in milliseconds, if tll is 0,
	 *  expiration will not be set
	 * @param replace {bool} 如果 key 存在是否直接覆盖
	 *  if the key already exists, this parameter decides if replacing
	 *  the existing key
	 * @return {bool}
	 *  true on success, false on error
	 */
	bool restore(const char* key, const char* value, size_t len,
		int ttl, bool replace = false);

	/**
	 * 获得 KEY 的剩余生存周期，单位（秒）
	 * get the time to live for a key in seconds
	 * @param key {const char*} 键值
	 *  the key
	 * @return {int} 返回对应键值的生存周期
	 *  return value as below:
	 *  > 0: 该 key 剩余的生存周期（秒）
	 *       the time to live for a key in seconds
	 *   -3：出错
	 *       error happened
	 *   -2：key 不存在
	 *       the key doesn't exist
	 *   -1：当 key 存在但没有设置剩余时间
	 *       the key were not be set expiration
	 * 注：对于 redis-server 2.8 以前版本，key 不存在或存在但未设置生存期则返回 -1
	 * notice: for the redis version before 2.8, -1 will be returned
	 *  if the key doesn't exist or the key were not be set expiration
	 */
	int ttl(const char* key);

	/**
	 * 获得 KEY 的存储类型
	 * get the the type stored at key
	 * @para key {const char*} KEY 值
	 *  the key
	 * @return {redis_key_t} 返回 KEY 的存储类型
	 *  return redis_key_t defined above as REDIS_KEY_
	 */
	redis_key_t type(const char* key);

	/**
	 * 命令用于迭代当前数据库中的数据库键
	 * incrementally iterate the keys space in the specified database
	 * @param cursor {int} 游标值，开始遍历时该值写 0
	 *  the iterating cursor beginning with 0
	 * @param out {std::vector<acl::string>&} 存储结果集，内部以追加方式将本次
	 *  遍历结果集合添加进该数组中，为防止因总结果集过大导致该数组溢出，用户可在
	 *  调用本函数前后清理该数组对象
	 *  string array storing the results, the array will be cleared
	 *  internal and the string result will be appened to the array
	 * @param pattern {const char*} 匹配模式，glob 风格，非空时有效
	 &  the matching pattern with glob style, only effective if not NULL
	 * @param count {const size_t*} 限定的结果集数量，非空指针时有效
	 *  limit the max number of the results stored in array, only
	 *  effective when not NULL
	 * @return {int} 下一个游标位置，含义如下：
	 *  return the next cursor value as follow:
	 *   0：遍历结束
	 *      iterating is finished
	 *  -1: 出错
	 *      some error happened
	 *  >0: 游标的下一个位置，即使这样，具体有多少结果还需要检查 out，因为有可能为空
	 *      the next cursor value for iterating
	 */
	int scan(int cursor, std::vector<string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl
