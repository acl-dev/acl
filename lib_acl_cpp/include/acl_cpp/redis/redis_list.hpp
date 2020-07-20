#pragma once
#include "../acl_cpp_define.hpp"
#include "redis_command.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class redis_client;

class ACL_CPP_API redis_list : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_list(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_list(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*)
	 */
	redis_list(redis_client_cluster* cluster);

	ACL_CPP_DEPRECATED
	redis_list(redis_client_cluster* cluster, size_t max_conns);

	virtual ~redis_list(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 从 key 列表对象中弹出一个元素对象（name/value对），采用阻塞方式从头部弹出；
	 * 当给定多个 key 参数时，按参数 key 的先后顺序依次检查各个列表，弹出第一个
	 * 非空列表的头元素
	 * remove and get a element from list head, or block until one
	 * is available; when multiple keys were given, multiple elements
	 * will be gotten according the sequence of keys given.
	 * @param result {std::pair<string, string>&} 存储结果元素对象，该对象的
	 *  第一个字符串表示是列表对象的 key，第二个为该对象的头部元素
	 *  store the elements result, the first string of pair is the key,
	 *  and the second string of pair is the element
	 * @param timeout {size_t} 等待阻塞时间（秒），在超时时间内没有获得元素对象，
	 *  则返回 false；如果该值为 0 则一直等待至获得元素对象或出错
	 *  the blocking timeout in seconds before one element availble;
	 *  false will be returned when the timeout is arrived; if the timeout
	 *  was set to be 0, this function will block until a element was
	 *  available or some error happened.
	 * @param first_key {const char*} 第一个非空字符串的 key 键，最后一个参数
	 *  必须以 NULL 结束，表示变参列表的结束
	 *  the first key of a variable args, the last arg must be NULL
	 *  indicating the end of the variable args.
	 * @return {bool} 是否获得了头部元素对象，如果返回 false 则有以下可能原因：
	 *  true if got a element in the head of list, when false was be
	 *  returned, there'are some reasons show below:
	 *  1、出错
	 *     error happened.
	 *  2、有一个 key 非列表对象
	 *     at least one key was not a list object.
	 *  3、key 不存在或超时未获得元素对象
	 *     key not exist or timeout was got.

	 */
	bool blpop(std::pair<string, string>& result, size_t timeout,
		const char* first_key, ...);
	bool blpop(const std::vector<const char*>& keys, size_t timeout,
		std::pair<string, string>& result);
	bool blpop(const std::vector<string>& keys, size_t timeout,
		std::pair<string, string>& result);

	/**
	 * 含义参见 blpop，唯一区别为该方法弹出尾部元素对象
	 * the meaning is same as the blpop above except that this function
	 * is used to pop element from the tail of the list
	 * @see blpop
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
	 * two actions will be executed in blocking mode as below:
	 * 1) pop a element from src list's tail, and return it to caller
	 * 2) push the element to dst list's head
	 * @param src {const char*} 源列表对象 key
	 *  the key of source list
	 * @param dst {const char*} 目标列表对象 key
	 *  the key of destination list
	 * @param buf {string*} 非空时存储 src 的尾部元素 key 值
	 *  if not NULL, buf will store the element poped from the head of src
	 * @param timeout {size_t} 等待超时时间，如果为 0 则一直等待直到有数据或出错
	 *  the timeout to wait, if the timeout is 0 this function will
	 *  block until a element was available or error happened.
	 * @return {bool} 当从 src 列表中成功弹出尾部元素并放入 dst 列表头部后
	 *  该方法返回 true；返回 false 表示超时、出错或 src/dst 有一个非列表对象
	 *  true if success, false if timeout arrived, or error happened,
	 *  or one of the src and dst is not a list object
	 * @see rpoplpush
	 */
	bool brpoplpush(const char* src, const char* dst, size_t timeout,
		string* buf = NULL);

	/**
	 * 返回 key 对应的列表对象中，指定下标的元素
	 * return the element of the specified subscript from the list at key
	 * @param key {const char*} 列表对象的 key
	 *  the key of one list object
	 * @param idx {size_t} 下标值
	 *  the specified subscript
	 * @param buf {string&} 存储结果
	 *  store the result
	 * @return {bool} 返回 true 表明操作成功，此时若 buf 数据非空则表明正确获得了
	 *  指定下标的元素，如果 buf.empty()表示没有获得元素；返回 false 时表明操作失败
	 *  true if success, and if buf is empty, no element was got;
	 *  false if error happened
	 */
	bool lindex(const char* key, size_t idx, string& buf);

	/**
	 * 在列表对象中将一个新元素添加至指定元素的前面
	 * insert one new element before the specified element in list
	 * @param key {const char*} 列表对象的 key
	 *  the key of specified list
	 * @param pivot {const char*} 列表对象中的一个指定元素名
	 *  the speicifed element of list
	 * @param value {const char*} 新的元素名
	 *  the new element to be inserted
	 * @reutrn {int} 返回该列表对象的元素个数，含义如下：
	 *  return the number of list specified by the given key, as below:
	 *  -1 -- 表示出错或 key 非列表对象
	 *        error happened or the object of the key is not a list
	 *  >0 -- 当前列表对象的元素个数
	 *        the number of elements of the specified list
	 */
	int linsert_before(const char* key, const char* pivot,
		const char* value);
	int linsert_before(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	/**
	 * 在列表对象中将一个新元素添加至指定元素的后面
	 * append a new element after the specified element in the list
	 * @param key {const char*} 列表对象的 key
	 *  the key of the specified list
	 * @param pivot {const char*} 列表对象中的一个指定元素名
	 *  the specified element
	 * @param value {const char*} 新的元素名
	 *  the new element
	 * @reutrn {int} 返回该列表对象的元素个数，含义如下：
	 *  return the number of elements in the list specifed by the key:
	 *  -1 -- 表示出错或 key 非列表对象
	 *        error happened or it is not a list object specified by key
	 *  >0 -- 当前列表对象的元素个数
	 *        the number of elements in list specified by the key
	 */
	int linsert_after(const char* key, const char* pivot,
		const char* value);
	int linsert_after(const char* key, const char* pivot,
		size_t pivot_len, const char* value, size_t value_len);

	/**
	 * 返回指定列表对象的元素个数
	 * get the number of elements in list specified the given key
	 * @param key {const char*} 列表对象的 key
	 *  the list's key
	 * @return {int} 返回指定列表对象的长度（即元素个数）， -1 if error happened
	 *  return the number of elements in list, -1 if error
	 */
	int llen(const char* key);

	/**
	 * 从列表对象中移除并返回头部元素
	 * remove and get the element in the list's head
	 * @param key {const char*} 元素对象的 key
	 *  the key of one list
	 * @param buf {string&} 存储弹出的元素值
	 *  store the element when successful.
	 * @return {int} 返回值含义：>0 -- 表示成功弹出一个元素且返回值表示元素的长度，
	 *  -1 -- 表示出错，或该对象非列表对象，或该对象已经为空
	 *  return value as below:
	 *   >0: get one element successfully and return the length of element
	 *  -1: error happened, or the oject is not a list specified
	 *      by the key, or the list specified by key is empty
	 */
	int lpop(const char* key, string& buf);

	/**
	 * 将一个或多个值元素插入到列表对象 key 的表头
	 * add one or more element(s) to the head of a list
	 * @param key {const char*} 列表对象的 key
	 *  the list key
	 * @param first_value {const char*} 第一个非空字符串，该变参的列表的最后一个
	 *  必须设为 NULL
	 *  the first no-NULL element of the variable args, the last arg must
	 *  be NULL indicating the end of the args.
	 * @return {int} 返回添加完后当前列表对象中的元素个数，返回 -1 表示出错或该 key
	 *  对象非列表对象，当该 key 不存在时会添加新的列表对象及对象中的元素
	 *  return the number of elements in list. -1 if error happened,
	 *  or the object specified by key is not a list.
	 */
	int lpush(const char* key, const char* first_value, ...);
	int lpush(const char* key, const char* values[], size_t argc);
	int lpush(const char* key, const std::vector<string>& values);
	int lpush(const char* key, const std::vector<const char*>& values);
	int lpush(const char* key, const char* values[], const size_t lens[],
		size_t argc);

	/**
	 * 将一个新的列表对象的元素添加至已经存在的指定列表对象的头部，当该列表对象
	 * 不存在时则不添加
	 * add a new element before the head of a list, only if the list exists
	 * @param key {const char*} 列表对象的 key
	 *  the list's key
	 * @param value {const char*} 新加的列表对象的元素
	 *  the new element to be added
	 * @return {int} 返回当前列表对象的元素个数，含义如下：
	 *  return the number of elements in the list:
	 *  -1：出错，或该 key 非列表对象
	 *      error or the key isn't refer to a list
	 *   0：该 key 对象不存在
	 *      the list specified by the given key doesn't exist
	 *  >0：添加完后当前列表对象中的元素个数
	 *      the number of elements in list specifed by key after added
	 */
	int lpushx(const char* key, const char* value);
	int lpushx(const char* key, const char* value, size_t len);

	/**
	 * 返回列表 key 中指定区间内（闭区间）的元素，区间以偏移量 start 和 end 指定；
	 * 下标起始值从 0 开始，-1 表示最后一个下标值
	 * get a range of elements from list, the range is specified by
	 * start and end, and the start begins with 0, -1 means the end
	 * @param key {const char*} 列表对象的 key
	 *  the specified key of one list
	 * @param start {int} 起始下标值
	 *  the start subscript of list
	 * @param end {int} 结束下标值
	 *  the end subscript of list
	 * @param result {std::vector<string>*} 非空时存储列表对象中指定区间的元素集合
	 *  if not NULL, result will be used to store the results
	 * @return {bool} 操作是否成功，当返回 false 表示出错或 key 非列表对象
	 *  if success for this operation, false if the key is not a list or
	 *  error happened
	 *  举例：
	 *  for example:
	 *  1) 当 start = 0, end = 10 时则指定从下标 0 开始至 10 的 11 个元素
	 *     if start is 0 and end is 10, then the subscript range is
	 *     between 0 and 10(include 10).
	 *  2) 当 start = -1, end = -2 时则指定从最后一个元素第倒数第二个共 2 个元素 
	 *     if start is -1 and end is -2, the range is from the end and
	 *     backward the second element.
	 *
	 *  操作成功后可以通过以下任一方式获得数据
	 *  the result can be got by one of the ways as below:
	 *
	 *  1、在调用方法中传入非空的存储结果对象的地址
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function
	 *  2、基类方法 get_value 获得指定下标的元素数据
	 *     get the specified subscript's element by redis_command::get_value 
	 *  3、基类方法 get_child 获得指定下标的元素对象(redis_result），然后再通过
	 *     redis_result::argv_to_string 方法获得元素数据
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 *  4、基类方法 get_result 方法取得总结果集对象 redis_result，然后再通过
	 *     redis_result::get_child 获得一个元素对象，然后再通过方式 2 中指定
	 *     的方法获得该元素的数据
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above.
	 *  5、基类方法 get_children 获得结果元素数组对象，再通过 redis_result 中
	 *     的方法 argv_to_string 从每一个元素对象中获得元素数据
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string.
	 */
	bool lrange(const char* key, int start, int end,
		std::vector<string>* result);

	/**
	 * 根据元素值从列表对象中移除指定数量的元素
	 * remove the first count occurrences of elements equal to value
	 * from the list stored at key
	 * @param key {const char*} 列表对象的 key
	 *  the key of a list
	 * @param count {int} 移除元素的数量限制，count 的含义如下：
	 *  the first count of elements to be removed, as below:
	 *  count > 0 : 从表头开始向表尾搜索，移除与 value 相等的元素，数量为 count
	 *              remove elements equal to value moving from head to tail
	 *  count < 0 : 从表尾开始向表头搜索，移除与 value 相等的元素，数量为 count 的绝对值
	 *              remove elements equal to value moving from tail to head
	 *  count = 0 : 移除表中所有与 value 相等的值
	 *              remove all elements equal to value
	 * @param value {const char*} 指定的元素值，需要从列表对象中遍历所有与该值比较
	 *  the specified value for removing elements
	 * @return {int} 被移除的对象数量，返回值含义如下：
	 *  the count of elements removed, meaning show below:
	 *  -1：出错或该 key 对象非列表对象
	 *      error happened or the key is not refer to a list
	 *   0：key 不存在或移除的元素个数为 0
	 *      the key does not exist or the count of elements removed is 0
	 *  >0：被成功移除的元素数量
	 *      the count of elements removed successfully
	 */
	int lrem(const char* key, int count, const char* value);
	int lrem(const char* key, int count, const char* value, size_t len);

	/**
	 * 将列表 key 下标为 idx 的元素的值设置为 value，当 idx 参数超出范围，或对
	 * 一个空列表( key 不存在)进行 lset 时，返回一个错误
	 * set the value of a element in a list by its index, if the index
	 * out of bounds or the key of list not exist, an error will happen.
	 * @param key {const char*} 列表对象的 key
	 *  the key of list
	 * @param idx {int} 下标位置，当为负值时则从尾部向头尾部定位，否则采用顺序方式；
	 *  如：0 表示头部第一个元素，-1 表示尾部开始的第一个元素
	 *  the index in the list, if it's negative, iterating data will be
	 *  from tail to head, or be from head to tail.
	 * @param value {const char*} 元素新值
	 *  the new value of the element by its index
	 * @return {bool} 当 key 非列表对象或 key 不存在或 idx 超出范围则返回 false
	 *  if success. false if the object of the key isn't list, or key's
	 *  list not exist, or the index out of bounds.
	 */
	bool lset(const char* key, int idx, const char* value);
	bool lset(const char* key, int idx, const char* value, size_t len);

	/**
	 * 对指定的列表对象，对一个列表进行修剪，让列表只保留指定区间内的元素，
	 * 不在指定区间之内的元素都将被删除；区间以偏移量 start 和 end 指定；
	 * 下标起始值从 0 开始，-1 表示最后一个下标值
	 * remove elements in a list by range betwwen start and end.
	 * @param key {const char*} 列表对象的 key
	 *  the key of a list
	 * @param start {int} 起始下标值
	 *  the start index in a list
	 * @param end {int} 结束下标值
	 *  the end index in a list
	 * @return {bool} 操作是否成功，当返回 false 时表示出错或指定的 key 对象非
	 *  列表对象；当成功删除或 key 对象不存在时则返回 true
	 *  if success. false if error happened, or the key's object is not
	 *  a list, or the key's object not exist.
	 */
	bool ltrim(const char* key, int start, int end);

	/**
	 * 从列表对象中移除并返回尾部元素
	 * remove and get the last element of a list
	 * @param key {const char*} 元素对象的 key
	 *  the key of the list
	 * @param buf {string&} 存储弹出的元素值
	 *  store the element pop from list
	 * @return {int} 返回值含义：>0 -- 表示成功弹出一个元素且返回值表示元素的长度，
	 *  -1 -- 表示出错，或该对象非列表对象，或该对象已经为空
	 *  return value as below:
	 *   >0: get one element successfully and return the length of element
	 *  -1: error happened, or the oject is not a list specified
	 *      by the key, or the list specified by key is empty
	 */
	int rpop(const char* key, string& buf);

	/**
	 * 在一个原子时间内，非阻塞方式执行以下两个动作：
	 * 将列表 src 中的最后一个元素(尾元素)弹出，并返回给客户端。
	 * 将 src 弹出的元素插入到列表 dst ，作为 dst 列表的的头元素
	 * remove the last element in a list, prepend it to another list
	 * and return it.
	 * @param src {const char*} 源列表对象 key
	 *  the key of the source list
	 * @param dst {const char*} 目标列表对象 key
	 *  the key of the destination list
	 * @param buf {string*} 非空时存储 src 的尾部元素 key 值
	 *  if not NULL, it will store the element
	 * @return {bool} 当从 src 列表中成功弹出尾部元素并放入 dst 列表头部后
	 *  该方法返回 true；返回 false 出错或 src/dst 有一个非列表对象
	 *  true if the element was removed from a list to another list,
	 *  false if error happened, one of src or dst is not a list.
	 */
	bool rpoplpush(const char* src, const char* dst, string* buf = NULL);

	/**
	 * 将一个或多个值元素插入到列表对象 key 的表尾
	 * append one or multiple values to a list
	 * @param key {const char*} 列表对象的 key
	 *  the key of a list
	 * @param first_value {const char*} 第一个非空字符串，该变参的列表的最后一个
	 *  必须设为 NULL
	 *  the first element of a variable args must be not NULL, and the
	 *  last arg must be NULL indicating the end of the args.
	 * @return {int} 返回添加完后当前列表对象中的元素个数，返回 -1 表示出错或该 key
	 *  对象非列表对象，当该 key 不存在时会添加新的列表对象及对象中的元素
	 *  return the number of a list specified by a key. -1 if error
	 *  happened, or the key's object isn't a list, if the list by the
	 *  key doese not exist, a new list will be created with the key.
	 */
	int rpush(const char* key, const char* first_value, ...);
	int rpush(const char* key, const char* values[], size_t argc);
	int rpush(const char* key, const std::vector<string>& values);
	int rpush(const char* key, const std::vector<const char*>& values);
	int rpush(const char* key, const char* values[], const size_t lens[],
		size_t argc);

	/**
	 * 将一个新的列表对象的元素添加至已经存在的指定列表对象的尾部，当该列表对象
	 * 不存在时则不添加
	 * append one or multiple values to a list only if the list exists.
	 * @param key {const char*} 列表对象的 key
	 *  the key of a list
	 * @param value {const char*} 新加的列表对象的元素
	 *  the new element to be added.
	 * @return {int} 返回当前列表对象的元素个数，含义如下：
	 *  return the number of the list, as below:
	 *  -1：出错，或该 key 非列表对象
	 *      error happened, or the key's object isn't a list
	 *   0：该 key 对象不存在
	 *      the key's object doesn't exist
	 *  >0：添加完后当前列表对象中的元素个数
	 *     the number of elements in the list after adding.
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

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
