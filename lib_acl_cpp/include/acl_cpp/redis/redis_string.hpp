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

/**
 * 所有的字符串对象的命令都已实现
 */
class ACL_CPP_API redis_string : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_string();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_string(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_cluster*， size_t)
	 */
	redis_string(redis_cluster* cluster, size_t max_conns);
	virtual ~redis_string();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将字符串值 value 关联到 key
	 * @param key {const char*} 字符串对象的 key
	 * @param value {const char*} 字符串对象的 value
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 */
	bool set(const char* key, const char* value);
	bool set(const char* key, size_t key_len,
		const char* value, size_t value_len);

	/**
	 * 将值 value 关联到 key ，并将 key 的生存时间设为 timeout (以秒为单位)，
	 * 如果 key 已经存在， SETEX 命令将覆写旧值
	 * @param key {const char*} 字符串对象的 key
	 * @param value {const char*} 字符串对象的 value
	 * @param timeout {int} 过期值，单位为秒
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 */
	bool setex(const char* key, const char* value, int timeout);
	bool setex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * 将值 value 关联到 key ，并将 key 的生存时间设为 timeout (以毫秒为单位)，
	 * 如果 key 已经存在， SETEX 命令将覆写旧值
	 * @param key {const char*} 字符串对象的 key
	 * @param value {const char*} 字符串对象的 value
	 * @param timeout {int} 过期值，单位为毫秒
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 对象非字符串对象
	 */
	bool psetex(const char* key, const char* value, int timeout);
	bool psetex(const char* key, size_t key_len, const char* value,
		size_t value_len, int timeout);

	/**
	 * 将 key 的值设为 value ，当且仅当 key 不存在，若给定的 key 已经存在，
	 * 则 SETNX 不做任何动作
	 * @param key {const char*} 字符串对象的 key
	 * @param value {const char*} 字符串对象的 value
	 * @return {int} 返回值含义如下：
	 *  -1：出错或 key 非字符串对象
	 *   0：给定 key 的对象存在
	 *   1：添加成功
	 */
	int setnx(const char* key, const char* value);
	int setnx(const char* key, size_t key_len,
		const char* value, size_t value_len);

	/**
	 * 如果 key 已经存在并且是一个字符串， APPEND 命令将 value 追加到 key 原来
	 * 的值的末尾；如果 key 不存在， APPEND 就简单地将给定 key 设为 value
	 * @param key {const char*} 字符串对象的 key
	 * @param value {const char*} 字符串对象的值
	 * @return {int} 返回当前该字符串的长度，-1 表示出错或 key 非字符串对象
	 */
	int append(const char* key, const char* value);
	int append(const char* key, const char* value, size_t size);

	/**
	 * 返回 key 所关联的字符串值
	 * @param key {const char*} 字符串对象的 key
	 * @param buf {string&} 操作成功后存储字符串对象的值
	 * @return {bool} 操作是否成功，返回 false 表示出错或 key 非字符串对象
	 */
	bool get(const char* key, string& buf);
	bool get(const char* key, size_t len, string& buf);

	/**
	 * 返回 key 所关联的字符串值，当返回的字符串值比较大时，内部会自动进行切片，即将
	 * 一个大内存切成一些非连续的小内存，使用者需要根据返回的结果对象重新对结果数据进行
	 * 组装，比如可以调用： redis_result::get(size_t, size_t*) 函数获得某个切
	 * 片的片断数据，根据 redis_result::get_size() 获得分片数组的长度
	 * @param key {const char*} 字符串对象的 key
	 * @param buf {string&} 操作成功后存储字符串对象的值
	 * @return {bool} 操作是否成功，返回 false 表示出错或 key 非字符串对象
	 */
	const redis_result* get(const char* key);
	const redis_result* get(const char* key, size_t len);

	/**
	 * 将给定 key 的值设为 value ，并返回 key 的旧值，当 key 存在但不是
	 * 字符串类型时，返回一个错误
	 * @param key {const char*} 字符串对象的 key
	 * @param value {const char*} 设定的新的对象的值
	 * @param buf {string&} 存储对象的旧的值
	 * @return {bool} 是否成功
	 */
	bool getset(const char* key, const char* value, string& buf);
	bool getset(const char* key, size_t key_len, const char* value,
		size_t value_len, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 获得指定 key 的字符串对象的值的数据长度
	 * @param key {const char*} 字符串对象的 key
	 * @return {int} 返回值含义如下：
	 *  -1：出错或非字符串对象
	 *   0：该 key 不存在
	 *  >0：该字符串数据的长度
	 */
	int get_strlen(const char* key);
	int get_strlen(const char* key, size_t len);

	/**
	 * 用 value 参数覆写(overwrite)给定 key 所储存的字符串值，从偏移量 offset 开始，
	 * 不存在的 key 当作空白字符串处理
	 * @param key {const char*} 字符串对象的 key
	 * @param offset {unsigned} 偏移量起始位置，该值可以大于字符串的数据长度，此时
	 *  是间的空洞将由 \0 补充
	 * @param value {const char*} 覆盖的值
	 * @return {int} 当前字符串对象的数据长度
	 */
	int setrange(const char* key, unsigned offset, const char* value);
	int setrange(const char* key, size_t key_len, unsigned offset,
		const char* value, size_t value_len);

	/**
	 * 返回 key 中字符串值的子字符串，字符串的截取范围由 start 和 end 两个偏移量决定
	 * (包括 start 和 end 在内)
	 * @param key {const char*} 字符串对象的 key
	 * @param start {int} 起始下标值
	 * @param end {int} 结束下标值
	 * @param buf {string&} 成功时存储结果
	 * @return {bool} 操作是否成功
	 *  注：下标位置可以为负值，表示从字符串尾部向前开始计数，如 -1 表示最后一个元素
	 */
	bool getrange(const char* key, int start, int end, string& buf);
	bool getrange(const char* key, size_t key_len,
		int start, int end, string& buf);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 对 key 所储存的字符串值，设置或清除指定偏移量上的位(bit)，
	 * 位的设置或清除取决于 value 参数，可以是 0 也可以是 1
	 * @param key {const char*} 字符串对象的 key
	 * @param offset {unsigned} 指定偏移量的位置
	 * @param bit {bool} 为 true 表示设置标志位，否则为取消标志位
	 * @return {int} 操作成功后返回该位置原来的存储位的值，含义如下：
	 *  -1：出错或该 key 非字符串对象
	 *   0：原来的存储位为 0
	 *   1：原来的存储位为 1
	 */
	bool setbit(const char* key, unsigned offset, bool bit);
	bool setbit(const char* key, size_t len, unsigned offset, bool bit);

	/**
	 * 对 key 所储存的字符串值，获取指定偏移量上的位(bit)，当 offset 比字符串值
	 * 的长度大，或者 key 不存在时，返回 0
	 * @param key {const char*} 字符串对象的 key
	 * @param offset {unsigned} 指定偏移量的位置
	 * @param bit {int&} 成功后存储指定位置的标志位
	 * @return {bool} 操作是否成功，返回 false 表示出错或该 key 非字符串对象
	 */
	bool getbit(const char* key, unsigned offset, int& bit);
	bool getbit(const char* key, size_t len, unsigned offset, int& bit);

	/**
	 * 计算给定字符串中，被设置为 1 的比特位的数量，若指定了 start/end，则计数在指定
	 * 区间内进行
	 * @param key {const char*} 字符串对象的 key
	 * @return {int} 标志位为 1 的数量，-1 表示出错或非字符串对象
	 */
	int bitcount(const char* key);
	int bitcount(const char* key, size_t len);
	int bitcount(const char* key, int start, int end);
	int bitcount(const char* key, size_t len, int start, int end);

	/**
	 * 对一个或多个 key 求逻辑并，并将结果保存到 destkey
	 * @param destkey {const char*} 目标字符串对象的 key
	 * @param keys 源字符串对象集合
	 * @return {int}
	 */
	int bitop_and(const char* destkey, const std::vector<string>& keys);
	int bitop_and(const char* destkey, const std::vector<const char*>& keys);
	int bitop_and(const char* destkey, const char* key, ...);
	int bitop_and(const char* destkey, const char* keys[], size_t size);

	/**
	 * 对一个或多个 key 求逻辑或，并将结果保存到 destkey
	 * @param destkey {const char*} 目标字符串对象的 key
	 * @param keys 源字符串对象集合
	 * @return {int}
	 */
	int bitop_or(const char* destkey, const std::vector<string>& keys);
	int bitop_or(const char* destkey, const std::vector<const char*>& keys);
	int bitop_or(const char* destkey, const char* key, ...);
	int bitop_or(const char* destkey, const char* keys[], size_t size);

	/**
	 * 对一个或多个 key 求逻辑异或，并将结果保存到 destkey
	 * @param destkey {const char*} 目标字符串对象的 key
	 * @param keys 源字符串对象集合
	 * @return {int}
	 */
	int bitop_xor(const char* destkey, const std::vector<string>& keys);
	int bitop_xor(const char* destkey, const std::vector<const char*>& keys);
	int bitop_xor(const char* destkey, const char* key, ...);
	int bitop_xor(const char* destkey, const char* keys[], size_t size);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 同时设置一个或多个 key-value 对
	 * @param objs key-value 对集合
	 * @return {bool} 操作是否成功
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
	 * @param objs key-value 对集合
	 * @return {int} 返回值含义如下：
	 *  -1：出错或非字符串对象
	 *   0：添加的 key 集合中至少有一个已经存在
	 *   1：添加成功
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
	 * @param keys {const std::vector<string>&} 字符串 key 集合
	 * @param out {std::vector<string>*} 非空时存储字符串值集合数组，对于不存在
	 *  的 key 也会存储一个空串对象
	 * @return {bool} 操作是否成功，操作成功后可以通过以下任一种方式获得数据：
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
	 * @param key {const char*} 字符串对象的 key
	 * @param result {long long int*} 非空时存储操作结果
	 * @return {bool} 操作是否成功
	 */
	bool incr(const char* key, long long int* result = NULL);

	/**
	 * 将 key 所储存的值加上增量 increment
	 * 1）如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 INCRBY 命令
	 * 2）如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误
	 * 3）本操作的值限制在 64 位(bit)有符号数字表示之内
	 * @param key {const char*} 字符串对象的 key
	 * @param inc {long long int} 增量值
	 * @param result {long long int*} 非空时存储操作结果
	 * @return {bool} 操作是否成功
	 */
	bool incrby(const char* key, long long int inc,
		long long int* result = NULL);

	/**
	 * 为 key 中所储存的值加上浮点数增量
	 * 1) 如果 key 不存在，那么 INCRBYFLOAT 会先将 key 的值设为 0 ，再执行加法操作
	 * 2) 如果命令执行成功，那么 key 的值会被更新为（执行加法之后的）新值，并且新值会
	 *    以字符串的形式返回给调用者
	 * 3) 计算结果也最多只能表示小数点的后十七位
	 * @param key {const char*} 字符串对象的 key
	 * @param inc {double} 增量值
	 * @param result {double*} 非空时存储操作结果
	 * @return {bool} 操作是否成功
	 */
	bool incrbyfloat(const char* key, double inc, double* result = NULL);

	/**
	 * 将 key 中储存的数字值减一
	 * 1) 如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 DECR 操作
	 * 2) 如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误
	 * 3) 本操作的值限制在 64 位(bit)有符号数字表示之内
	 * @param key {const char*} 字符串对象的 key
	 * @param result {long long int*} 非空时存储操作结果
	 * @return {bool} 操作是否成功
	 */
	bool decr(const char* key, long long int* result = NULL);

	/**
	 * 将 key 所储存的值减去减量 decrement
	 * 1) 如果 key 不存在，那么 key 的值会先被初始化为 0 ，然后再执行 DECRBY 操作
	 * 2) 如果值包含错误的类型，或字符串类型的值不能表示为数字，那么返回一个错误
	 * 3) 本操作的值限制在 64 位(bit)有符号数字表示之内
	 * @param key {const char*} 字符串对象的 key
	 * @param dec {long long int} 减量值
	 * @param result {long long int*} 非空时存储操作结果
	 * @return {bool} 操作是否成功
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

	bool incoper(const char* cmd, const char* key, long long int inc,
		long long int* result);

};

} // namespace acl
