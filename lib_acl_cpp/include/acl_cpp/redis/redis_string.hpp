#pragma once
#include "../acl_cpp_define.hpp"
#include <map>
#include <vector>
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class string;
class redis_client;
class redis_result;

/**
 * 所有的字符串对象的命令都已实现
 * all the commands in redis Strings are be implemented.
 */
class ACL_CPP_API redis_string : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_string(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_string(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_string(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_string(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_string(void);

	/////////////////////////////////////////////////////////////////////
	
	/**
	 * 将字符串值 value 关联到 key
	 * set the string value of a key
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param value {const char*} 字符串对象的 value
	 *  the value of a string
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 *  true if SET was executed correctly, false if error happened or
	 *  the key's object isn't a string.
	 */
	bool set(const char* key, const char* value);
	bool set(const char* key, size_t key_len,
		const char* value, size_t value_len);

	#define SETFLAG_EX 	0x02
	#define SETFLAG_PX	0x03
	#define SETFLAG_NX	0x08
	#define SETFLAG_XX	0x0C
	/**
	 * 从 Redis 2.6.12 版本开始， SET 命令的行为可以通过一系列参数来修改：
	 * EX seconds ： 将键的过期时间设置为 seconds 秒。 执行 SET key value
	 * EX seconds 的效果等同于执行 SETEX key seconds value 。
	 * PX milliseconds ： 将键的过期时间设置为 milliseconds 毫秒。
	 *   执行 SET key value PX milliseconds 的效果等同于执行
	 *     PSETEX key milliseconds value 。
	 * NX ：只在键不存在时，才对键进行设置操作。执行 SET key value NX 的
	 *  效果等同于执行 SETNX key value 。
	 * XX ：只在键已经存在时， 才对键进行设置操作。
	 * @Note 因为 SET 命令可以通过参数来实现 SETNX 、 SETEX 以及 PSETEX
	 * 命令的效果， 所以 Redis 将来的版本可能会移除并废弃 SETNX 、 SETEX
	 * 和 PSETEX 这三个命令。
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param value {const char*} 字符串对象的 value
	 *  the value of a string
	 * @param timeout {int} 过期值，单位为秒(EX)/毫秒(PX)
	 *  the timeout in seconds of a string
	 * @param flag {int} 标记，EX/PX | NX/XX
	 *  the flag of a string
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 *  true if SET was executed correctly, false if error happened or
	 *  the key's object isn't a string.
 	 */
	bool set(const char* key, const char* value, int timeout, int flag);
	bool set(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout, int flag);

	/**
	 * 将值 value 关联到 key ，并将 key 的生存时间设为 timeout (以秒为单位)，
	 * 如果 key 已经存在， SETEX 命令将覆写旧值
	 * set key to hold the strnig value, and set key to timeout after
	 * a given number of seconds.
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param value {const char*} 字符串对象的 value
	 *  the value of a string
	 * @param timeout {int} 过期值，单位为秒
	 *  the timeout in seconds of a string
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 *  true if SETEX was executed correctly, false if error happened
	 *  or the object specified by the key is not a string
	 */
	bool setex(const char* key, const char* value, int timeout);
	bool setex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * 将值 value 关联到 key ，并将 key 的生存时间设为 timeout (以毫秒为单位)，
	 * 如果 key 已经存在， SETEX 命令将覆写旧值
	 * set key to hold the string value, and set key to timeout after
	 * a given number of milliseconds.
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param value {const char*} 字符串对象的 value
	 *  the value of a string
	 * @param timeout {int} 过期值，单位为毫秒
	 *  the timeout in milliseconds of a string
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 *  true if SETEX was executed correctly, false if error happened
	 *  or the object specified by the key is not a string
	 */
	bool psetex(const char* key, const char* value, int timeout);
	bool psetex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * 将 key 的值设为 value ，当且仅当 key 不存在，若给定的 key 已经存在，
	 * 则 SETNX 不做任何动作
	 * set the value of a key, only if the key does not exist.
	 * @param key {const char*} 字符串对象的 key
	 *  the key of the string
	 * @param value {const char*} 字符串对象的 value
	 *  the value of the string
	 * @return {int} 返回值含义如下：
	 *  return the value as below:
	 *  -1：出错或 key 非字符串对象
	 *      error happened or the object by the key isn't a string
	 *   0：给定 key 的对象存在
	 *      the string of the key already exists
	 *   1：添加成功
	 *      the command was executed correctly
	 */
	int setnx(const char* key, const char* value);
	int setnx(const char* key, size_t key_len,
		const char* value, size_t value_len);

	/**
	 * 如果 key 已经存在并且是一个字符串， APPEND 命令将 value 追加到 key 原来
	 * 的值的末尾；如果 key 不存在， APPEND 就简单地将给定 key 设为 value
	 * append a value to a key
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param value {const char*} 字符串对象的值
	 *  the value to be appended to a key
	 * @return {int} 返回当前该字符串的长度，-1 表示出错或 key 非字符串对象
	 *  return the length of the string after appending, -1 if error
	 *  happened or the key's object isn't a string
	 */
	int append(const char* key, const char* value);
	int append(const char* key, const char* value, size_t size);

	/**
	 * 返回 key 所关联的字符串值
	 * get the value of a key 
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param buf {string&} 操作成功后存储字符串对象的值，如果返回 true 且
	 *  该缓冲区为空则表示对应 key 不存在
	 *  store the value of a key after GET executed correctly, key not
	 *  exist if the buf is empty when return true
	 * @return {bool} 操作是否成功，返回 false 表示出错或 key 非字符串对象
	 *  if the GET was executed correctly, false if error happened or
	 *  is is not a string of the key
	 */
	bool get(const char* key, size_t len, string& buf);
	bool get(const char* key, string& buf);

	/**
	 * 返回 key 所关联的字符串值，当返回的字符串值比较大时，内部会自动进行切片，即将
	 * 一个大内存切成一些非连续的小内存，使用者需要根据返回的结果对象重新对结果数据进行
	 * 组装，比如可以调用： redis_result::get(size_t, size_t*) 函数获得某个切
	 * 片的片断数据，根据 redis_result::get_size() 获得分片数组的长度
	 * @param key {const char*} 字符串对象的 key
	 * @return {bool} 操作是否成功，返回 false 表示出错或 key 非字符串对象
	 */
	const redis_result* get(const char* key);
	const redis_result* get(const char* key, size_t len);

	/**
	 * 将给定 key 的值设为 value ，并返回 key 的旧值，当 key 存在但不是
	 * 字符串类型时，返回一个错误
	 * set the string value of a key and and return its old value
	 * @param key {const char*} 字符串对象的 key
	 *  the key of string
	 * @param value {const char*} 设定的新的对象的值
	 *  the new string value of the key
	 * @param buf {string&} 存储对象的旧的值
	 *  store the old string value of the key
	 * @return {bool} 是否成功
	 *  if GETSET was executed correctly.
	 */
	bool getset(const char* key, const char* value, string& buf);
	bool getset(const char* key, size_t key_len, const char* value,
		size_t value_len, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 获得指定 key 的字符串对象的值的数据长度
	 * get the length of value stored in a key
	 * @param key {const char*} 字符串对象的 key
	 *  the key of the string
	 * @return {int} 返回值含义如下：
	 *  return value as below:
	 *  -1：出错或非字符串对象
	 *      error happened or the it isn't a string of the key
	 *   0：该 key 不存在
	 *      the key doesn't exist
	 *  >0：该字符串数据的长度
	 *      the length of the value stored in a key
	 */
	int get_strlen(const char* key);
	int get_strlen(const char* key, size_t key_len);

	/**
	 * 用 value 参数覆写(overwrite)给定 key 所储存的字符串值，从偏移量 offset 开始，
	 * 不存在的 key 当作空白字符串处理
	 * overwrite part of a string at key starting at the specified offset
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @param offset {unsigned} 偏移量起始位置，该值可以大于字符串的数据长度，此时
	 *  是间的空洞将由 \0 补充
	 *  the specified offset of the string
	 * @param value {const char*} 覆盖的值
	 *  the value to be set
	 * @return {int} 当前字符串对象的数据长度
	 *  the length of the string after SETRANGE
	 */
	int setrange(const char* key, unsigned offset, const char* value);
	int setrange(const char* key, size_t key_len, unsigned offset,
		const char* value, size_t value_len);

	/**
	 * 返回 key 中字符串值的子字符串，字符串的截取范围由 start 和 end 两个偏移量决定
	 * (包括 start 和 end 在内)
	 * get substring of the string stored at a key
	 * @param key {const char*} 字符串对象的 key
	 *  the key of string
	 * @param start {int} 起始下标值
	 *  the starting offset of the string
	 * @param end {int} 结束下标值
	 *  the ending offset of the string
	 * @param buf {string&} 成功时存储结果
	 *  store the substring result
	 * @return {bool} 操作是否成功
	 *  if GETRANGE was executed correctly.
	 *  注：下标位置可以为负值，表示从字符串尾部向前开始计数，如 -1 表示最后一个元素
	 */
	bool getrange(const char* key, int start, int end, string& buf);
	bool getrange(const char* key, size_t key_len,
		int start, int end, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 对 key 所储存的字符串值，设置或清除指定偏移量上的位(bit)，
	 * 位的设置或清除取决于 value 参数，可以是 0 也可以是 1
	 * set or clear the bit at offset in the string value stored at key
	 * @param key {const char*} 字符串对象的 key
	 *  the key of the string
	 * @param offset {unsigned} 指定偏移量的位置
	 *  the offset at the string value
	 * @param bit {bool} 为 true 表示设置标志位，否则为取消标志位
	 *  set bit if true, or clear bit if false at the specified offset
	 * @return {bool} 操作是否成功
	 *  if the command was executed correctly
	 */
	bool setbit_(const char* key, unsigned offset, bool bit);
	bool setbit_(const char* key, size_t len, unsigned offset, bool bit);

	/**
	 * 对 key 所储存的字符串值，获取指定偏移量上的位(bit)，当 offset 比字符串值
	 * 的长度大，或者 key 不存在时，返回 0
	 * get the bit at offset in the string value stored at key
	 * @param key {const char*} 字符串对象的 key
	 *  the key of the string
	 * @param offset {unsigned} 指定偏移量的位置
	 *  the offset in the string value
	 * @param bit {int&} 成功后存储指定位置的标志位
	 *  on success it will stored the bit at the specified offset
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 非字符串对象
	 *  if the GETBIT was executed correctly, false if error happened,
	 *  or the key doesn't store a string object
	 */
	bool getbit(const char* key, unsigned offset, int& bit);
	bool getbit(const char* key, size_t len, unsigned offset, int& bit);

	/**
	 * 计算给定字符串中，被设置为 1 的比特位的数量，若指定了 start/end，则计数在指定
	 * 区间内进行
	 * count set bits in a string
	 * @param key {const char*} 字符串对象的 key
	 *  the key of a string
	 * @return {int} 标志位为 1 的数量，-1 表示出错或非字符串对象
	 *  the count of bits been set, -1 if error happened or it's not
	 *  a string
	 */
	int bitcount(const char* key);
	int bitcount(const char* key, size_t len);
	int bitcount(const char* key, int start, int end);
	int bitcount(const char* key, size_t len, int start, int end);

	/**
	 * 对一个或多个 key 求逻辑并，并将结果保存到 destkey
	 * BITOP AND on multiple source keys and save the result to another key
	 * @param destkey {const char*} 目标字符串对象的 key
	 *  the key storing the result
	 * @param keys 源字符串对象集合
	 *  the source keys
	 * @return {int} 存储在目标 key 中的字符串的长度
	 *  the size of the string stored in the destination key, that is
	 *  equal to the size of the longest input string
	 */
	int bitop_and(const char* destkey, const std::vector<string>& keys);
	int bitop_and(const char* destkey, const std::vector<const char*>& keys);
	int bitop_and(const char* destkey, const char* key, ...);
	int bitop_and(const char* destkey, const char* keys[], size_t size);

	/**
	 * 对一个或多个 key 求逻辑或，并将结果保存到 destkey
	 * BITOP OR on multiple source keys and save the result to another key
	 * @param destkey {const char*} 目标字符串对象的 key
	 *  the destination key
	 * @param keys 源字符串对象集合
	 *  the source keys
	 * @return {int}
	 *  the size of the string stored in the destination key
	 */
	int bitop_or(const char* destkey, const std::vector<string>& keys);
	int bitop_or(const char* destkey, const std::vector<const char*>& keys);
	int bitop_or(const char* destkey, const char* key, ...);
	int bitop_or(const char* destkey, const char* keys[], size_t size);

	/**
	 * 对一个或多个 key 求逻辑异或，并将结果保存到 destkey
	 * BITOP XOR on multiple source keys and save the result to another key
	 * @param destkey {const char*} 目标字符串对象的 key
	 *  the destination key
	 * @param keys 源字符串对象集合
	 *  the source keys
	 * @return {int}
	 *  the size of the string stored in the destination key
	 */
	int bitop_xor(const char* destkey, const std::vector<string>& keys);
	int bitop_xor(const char* destkey, const std::vector<const char*>& keys);
	int bitop_xor(const char* destkey, const char* key, ...);
	int bitop_xor(const char* destkey, const char* keys[], size_t size);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 同时设置一个或多个 key-value 对
	 * set multiple key-value pair
	 * @param objs key-value 对集合
	 *  the collection of multiple key-value pair
	 * @return {bool} 操作是否成功
	 *  if the command was executed correctly
	 */
	bool mset(const std::map<string, string>& objs);
	bool mset(const std::vector<string>& keys,
		const std::vector<string>& values);
	bool mset(const char* keys[], const char* values[], size_t argc);
	bool mset(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 当且仅当所有给定 key 都不存在时同时设置一个或多个 key-value 对
	 * set multiple keys to multiple values only if none of the keys exist
	 * @param objs key-value 对集合
	 *  the collection of multile key-value pair
	 * @return {int} 返回值含义如下：
	 *  return value as below:
	 *  -1：出错或非字符串对象
	 *      error happened or there were a object of not a string.
	 *   0：添加的 key 集合中至少有一个已经存在
	 *     none be set because some of the keys already exist
	 *   1：添加成功
	 *     add ok.
	 */
	int msetnx(const std::map<string, string>& objs);
	int msetnx(const std::vector<string>& keys,
		const std::vector<string>& values);
	int msetnx(const char* keys[], const char* values[], size_t argc);
	int msetnx(const char* keys[], const size_t keys_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 返回所有(一个或多个)给定 key 的值，如果给定的 key 里面，有某个 key 不存在，
	 * 那么这个 key 返回空串添加进结果数组中
	 * get the values of the given keys
	 * @param keys {const std::vector<string>&} 字符串 key 集合
	 *  the given keys
	 * @param out {std::vector<acl::string>*} 非空时存储字符串值集合数组，
	 *  对于不存在的 key 也会存储一个空串对象
	 *  acl::string array storing the result. if one key not exists,
	 *  a empty string "" will also be stored in the array.
	 * @return {bool} 操作是否成功，操作成功后可以通过以下任一种方式获得数据：
	 *  if successul, one of below ways can be used to get the result:
	 *
	 *  1、在调用方法中传入非空的存储结果对象的地址
	 *     input the no-NULL result parameter when call hmget, when
	 *     success, the result will store the values of the given fileds
	 *
	 *  2、基类方法 get_value 获得指定下标的元素数据
	 *     call redis_command::result_value with the specified subscript
	 *
	 *  3、基类方法 get_child 获得指定下标的元素对象(redis_result），然后再通过
	 *     redis_result::argv_to_string 方法获得元素数据
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
	bool mget(const std::vector<string>& keys,
		std::vector<string>* out = NULL);
	bool mget(const std::vector<const char*>& keys,
		std::vector<string>* out = NULL);

	bool mget(std::vector<string>* result, const char* first_key, ...);
	bool mget(const char* keys[], size_t argc,
		std::vector<string>* out = NULL);
	bool mget(const char* keys[], const size_t keys_len[], size_t argc,
		std::vector<string>* out = NULL);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将 key 中储存的数字值增一
	 * 1）如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 INCR 操作；
	 * 2）如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误；
	 * 3）本操作的值限制在 64 位(bit)有符号数字表示之内
	 * increment the integer value of a key by one
	 * 1) if key not exists, the key's value will be set 0 and INCR
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} 字符串对象的 key
	 *  the given key
	 * @param result {long long int*} 非空时存储操作结果
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} 操作是否成功
	 *  if the INCR was executed correctly
	 */
	bool incr(const char* key, long long int* result = NULL);

	/**
	 * 将 key 所储存的值加上增量 increment
	 * 1）如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 INCRBY 命令
	 * 2）如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误
	 * 3）本操作的值限制在 64 位(bit)有符号数字表示之内
	 * increment the integer value of a key by a given amount
	 * 1) if key not exists, the key's value will be set 0 and INCRBY
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} 字符串对象的 key
	 *  the given key
	 * @param inc {long long int} 增量值
	 *  the given amount
	 * @param result {long long int*} 非空时存储操作结果
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} 操作是否成功
	 *  if the INCRBY was executed correctly
	 */
	bool incrby(const char* key, long long int inc,
		long long int* result = NULL);

	/**
	 * 为 key 中所储存的值加上浮点数增量
	 * 1) 如果 key 不存在，那么 INCRBYFLOAT 会先将 key 的值设为 0 ，再执行加法操作
	 * 2) 如果命令执行成功，那么 key 的值会被更新为（执行加法之后的）新值，并且新值会
	 *    以字符串的形式返回给调用者
	 * 3) 计算结果也最多只能表示小数点的后十七位
	 * increment the float value of a key by the given amount
	 * 1) if key not exists, the key's value will be set 0 and INCRBYFLOAT
	 * 2) if key's value is not a float an error will be returned
	 * @param key {const char*} 字符串对象的 key
	 *  the given key
	 * @param inc {double} 增量值
	 *  the given amount
	 * @param result {double*} 非空时存储操作结果
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} 操作是否成功
	 *  if the INCRBYFLOAT was executed correctly
	 */
	bool incrbyfloat(const char* key, double inc, double* result = NULL);

	/**
	 * 将 key 中储存的数字值减一
	 * 1) 如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 DECR 操作
	 * 2) 如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误
	 * 3) 本操作的值限制在 64 位(bit)有符号数字表示之内
	 * decrement the integer value of a key by one
	 * 1) if key not exists, the key's value will be set 0 and DECR
	 * 2) if key's value is not a number an error will be returned
	 * 3) the number is a 64 signed integer
	 * @param key {const char*} 字符串对象的 key
	 *  the given key
	 * @param result {long long int*} 非空时存储操作结果
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} 操作是否成功
	 *  if the DECR was executed correctly
	 */
	bool decr(const char* key, long long int* result = NULL);

	/**
	 * 将 key 所储存的值减去减量 decrement
	 * 1) 如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 DECRBY 操作
	 * 2) 如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误
	 * 3) 本操作的值限制在 64 位(bit)有符号数字表示之内
	 * decrement the integer value of a key by the given amount
	 * @param key {const char*} 字符串对象的 key
	 *  the given key
	 * @param dec {long long int} 减量值
	 *  the given amount
	 * @param result {long long int*} 非空时存储操作结果
	 *  store the result after INCR if it isn't NULL
	 * @return {bool} 操作是否成功
	 *  if the DECRBY was executed correctly
	 */
	bool decrby(const char* key, long long int dec,
		long long int* result = NULL);

private:
	int bitop(const char* op, const char* destkey,
		const std::vector<string>& keys);
	int bitop(const char* op, const char* destkey,
		const std::vector<const char*>& keys);
	int bitop(const char* op, const char* destkey,
		const char* keys[], size_t size);

	bool incoper(const char* cmd, const char* key, long long int* inc,
		long long int* result);

};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
