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

	/**
	 * 从 key 列表对象中弹出一个元素对象（name/value对），采用阻塞方式从头部弹出；
	 * 当给定多个 key 参数时，按参数 key 的先后顺序依次检查各个列表，弹出第一个
	 * 非空列表的头元素
	 * @param result {std::pair<string, string>&} 存储结果元素对象，该对象的
	 *  第一个字符串表示是列表对象的 key，第二个为该对象的头部元素
	 * @param timeout {size_t} 等待阻塞时间（秒），在超时时间内没有获得元素对象，
	 *  则返回 false；如果该值为 0 则一直等待至获得元素对象或出错
	 * @param first_key {const char*} 第一个非空字符串的 key 键
	 * @return {bool} 是否获得了头部元素对象，如果返回 false 则有以下可能原因：
	 *  1、出错
	 *  2、有一个 key 非列表对象
	 *  3、key 不存在或超时未获得元素对象

	 */
	bool blpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool blpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool blpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	/**
	 * 含义参见 blpop，唯一区别为该方法弹出尾部元素对象
	 * @sess blpop
	 */
	bool brpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool brpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool brpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	/**
	 * 阻塞式执行以下两个动作：
	 * 1) 将列表 src 中的最后一个元素(尾元素)弹出，并返回给客户端。
	 * 2) 将 src 弹出的元素插入到列表 dst ，作为 dst 列表的的头元素
	 * @param src {const char*} 源列表对象 key
	 * @param dst {const char*} 目标列表对象 key
	 * @param buf {string*} 非空时存储 src 的尾部元素 key 值
	 * @param timeout {size_t} 等待超时时间，如果为 0 则一直等待直到有数据或出错
	 * @return {bool} 当从 src 列表中成功弹出尾部元素并放入 dst 列表头部后
	 *  该方法返回 true；返回 false 表示超时、出错或 src/dst 有一个非列表对象
	 * @see rpoplpush
	 */
	bool brpoplpush(const char* src, const char* dst, size_t timeout,
		string* buf = NULL);

	/**
	 * 返回 key 对应的列表对象中，指定下标的元素
	 * @param key {const char*} 列表对象的 key
	 * @param idx {size_t} 下标值
	 * @param buf {string&} 存储结果
	 * @return {bool} 返回 true 表明操作成功，此时若 buf 数据非空则表明正确获得了
	 *  指定下标的元素，如果 buf.empty()表示没有获得元素；返回 false 时表明操作失败
	 */
	bool lindex(const char* key, size_t idx, string& buf);

	/**
	 * 在列表对象中将一个新元素添加至指定元素的前面
	 * @param key {const char*} 列表对象的 key
	 * @param pivot {const char*} 列表对象中的一个指定元素名
	 * @param value {const char*} 新的元素名
	 * @reutrn {int} 返回该列表对象的元素个数，含义如下：
	 *  -1 -- 表示出错或 key 非列表对象
	 *  >0 -- 当前列表对象的元素个数
	 */
	int linsert_before(const char* key, const char* pivot,
		const char* value);
	int linsert_before(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	/**
	 * 在列表对象中将一个新元素添加至指定元素的后面
	 * @param key {const char*} 列表对象的 key
	 * @param pivot {const char*} 列表对象中的一个指定元素名
	 * @param value {const char*} 新的元素名
	 * @reutrn {int} 返回该列表对象的元素个数，含义如下：
	 *  -1 -- 表示出错或 key 非列表对象
	 *  >0 -- 当前列表对象的元素个数
	 */
	int linsert_after(const char* key, const char* pivot,
		const char* value);
	int linsert_after(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	/**
	 * 返回指定列表对象的元素个数
	 * @param key {const char*} 列表对象的 key
	 * @return {int} 返回指定列表对象的长度（即元素个数），
	 */
	int llen(const char* key);

	/**
	 * 从列表对象中移除并返回头部元素
	 * @param key {const char*} 元素对象的 key
	 * @param buf {string&} 存储弹出的元素值
	 * @return {int} 返回值含义：1 -- 表示成功弹出一个元素，-1 -- 表示出错，或该
	 *  对象非列表对象，或该对象已经为空
	 */
	int lpop(const char* key, string& buf);

	/**
	 * 将一个或多个值元素插入到列表对象 key 的表头
	 * @param key {const char*} 列表对象的 key
	 * @param first_value {const char*} 第一个非空字符串，该变参的列表的最后一个
	 *  必须设为 NULL
	 * @return {int} 返回添加完后当前列表对象中的元素个数，返回 -1 表示出错或该 key
	 *  对象非列表对象，当该 key 不存在时会添加新的列表对象及对象中的元素
	 */
	int lpush(const char* key, const char* first_value, ...);
	int lpush(const char* key, const char* values[], size_t argc);
	int lpush(const char* key, const std::vector<string>& values);
	int lpush(const char* key, const std::vector<const char*>& values);
	int lpush(const char* key, const char* values[], size_t lens[],
		size_t argc);

	/**
	* 将一个新的列表对象的元素添加至已经存在的指定列表对象的头部，当该列表对象
	* 不存在时则不添加
	 * @param key {const char*} 列表对象的 key
	 * @param value {const char*} 新加的列表对象的元素
	 * @return {int} 返回当前列表对象的元素个数，含义如下：
	 *  -1：出错，或该 key 非列表对象
	 *   0：该 key 对象不存在
	 *  >0：添加完后当前列表对象中的元素个数
	 */
	int lpushx(const char* key, const char* value);
	int lpushx(const char* key, const char* value, size_t len);

	/**
	 * 返回列表 key 中指定区间内（闭区间）的元素，区间以偏移量 start 和 end 指定；
	 * 下标起始值从 0 开始，-1 表示最后一个下标值
	 * @param key {const char*} 列表对象的 key
	 * @param start {int} 起始下标值
	 * @param end {int} 结束下标值
	 * @param result {std::vector<string>&} 存储列表对象中指定区间的元素集合
	 * @return {bool} 操作是否成功，当返回 false 表示出错或 key 非列表对象
	 *  举例：
	 *  1) 当 start = 0, end = 10 时则指定从下标 0 开始至 10 的 11 个元素
	 *  2) 当 start = -1, end = -2 时则指定从最后一个元素第倒数第二个共 2 个元素 
	 */
	bool lrange(const char* key, int start, int end,
		std::vector<string>& result);

	/**
	 * 根据元素值从列表对象中移除指定数量的元素
	 * @param key {const char*} 列表对象的 key
	 * @param count {int} 移除元素的数量限制，count 的含义如下：
	 *  count > 0 : 从表头开始向表尾搜索，移除与 value 相等的元素，数量为 count
	 *  count < 0 : 从表尾开始向表头搜索，移除与 value 相等的元素，数量为 count 的绝对值
	 *  count = 0 : 移除表中所有与 value 相等的值
	 * @param value {const char*} 指定的元素值，需要从列表对象中遍历所有与该值比较
	 * @return {int} 被移除的对象数量，返回值含义如下：
	 *  -1：出错或该 key 对象非列表对象
	 *   0：key 不存在或移除的元素个数为 0
	 *  >0：被成功移除的元素数量
	 */
	int lrem(const char* key, int count, const char* value);
	int lrem(const char* key, int count, const char* value, size_t len);

	/**
	 * 将列表 key 下标为 idx 的元素的值设置为 value，当 idx 参数超出范围，或对
	 * 一个空列表( key 不存在)进行 lset 时，返回一个错误
	 * @param key {const char*} 列表对象的 key
	 * @param idx {int} 下标位置，当为负值时则从尾部向头尾部定位，否则采用顺序方式；
	 *  如：0 表示头部第一个元素，-1 表示尾部开始的第一个元素
	 * @param value {const char*} 元素新值
	 * @return {bool} 当 key 非列表对象或 key 不存在或 idx 超出范围则返回 false
	 */
	bool lset(const char* key, int idx, const char* value);
	bool lset(const char* key, int idx, const char* value, size_t len);

	/**
	 * 对指定的列表对象，根据限定区间范围进行删除；区间以偏移量 start 和 end 指定；
	 * 下标起始值从 0 开始，-1 表示最后一个下标值
	 * @param key {const char*} 列表对象的 key
	 * @param start {int} 起始下标值
	 * @param end {int} 结束下标值
	 * @return {bool} 操作是否成功，当返回 false 时表示出错或指定的 key 对象非
	 *  列表对象；当成功删除或 key 对象不存在时则返回 true
	 */
	bool ltrim(const char* key, int start, int end);

	/**
	 * 从列表对象中移除并返回尾部元素
	 * @param key {const char*} 元素对象的 key
	 * @param buf {string&} 存储弹出的元素值
	 * @return {int} 返回值含义：1 -- 表示成功弹出一个元素，-1 -- 表示出错，或该
	 *  对象非列表对象，或该对象已经为空
	 */
	int rpop(const char* key, string& buf);

	/**
	 * 在一个原子时间内，非阻塞方式执行以下两个动作：
	 * 将列表 src 中的最后一个元素(尾元素)弹出，并返回给客户端。
	 * 将 src 弹出的元素插入到列表 dst ，作为 dst 列表的的头元素
	 * @param src {const char*} 源列表对象 key
	 * @param dst {const char*} 目标列表对象 key
	 * @param buf {string*} 非空时存储 src 的尾部元素 key 值
	 * @return {bool} 当从 src 列表中成功弹出尾部元素并放入 dst 列表头部后
	 *  该方法返回 true；返回 false 出错或 src/dst 有一个非列表对象
	 */
	bool rpoplpush(const char* src, const char* dst, string* buf = NULL);

	/**
	 * 将一个或多个值元素插入到列表对象 key 的表尾
	 * @param key {const char*} 列表对象的 key
	 * @param first_value {const char*} 第一个非空字符串，该变参的列表的最后一个
	 *  必须设为 NULL
	 * @return {int} 返回添加完后当前列表对象中的元素个数，返回 -1 表示出错或该 key
	 *  对象非列表对象，当该 key 不存在时会添加新的列表对象及对象中的元素
	 */
	int rpush(const char* key, const char* first_value, ...);
	int rpush(const char* key, const char* values[], size_t argc);
	int rpush(const char* key, const std::vector<string>& values);
	int rpush(const char* key, const std::vector<const char*>& values);
	int rpush(const char* key, const char* values[], size_t lens[],
		size_t argc);

	/**
	 * 将一个新的列表对象的元素添加至已经存在的指定列表对象的尾部，当该列表对象
	 * 不存在时则不添加
	 * @param key {const char*} 列表对象的 key
	 * @param value {const char*} 新加的列表对象的元素
	 * @return {int} 返回当前列表对象的元素个数，含义如下：
	 *  -1：出错，或该 key 非列表对象
	 *   0：该 key 对象不存在
	 *  >0：添加完后当前列表对象中的元素个数
	 */
	int rpushx(const char* key, const char* value);
	int rpushx(const char* key, const char* value, size_t len);

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
	bool bpop(std::pair<string, string>& result);
};

} // namespace acl
