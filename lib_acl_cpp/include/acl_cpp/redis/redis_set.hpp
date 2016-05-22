#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_set : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_set(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_set(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*, size_t)
	 */
	redis_set(redis_client_cluster* cluster, size_t max_conns = 0);

	virtual ~redis_set(void);

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将一个或多个 member 元素加入到集合 key 当中，已经存在于集合的 member 元素
	 * 将被忽略;
	 * 1) 假如 key 不存在，则创建一个只包含 member 元素作成员的集合
	 * 2) 当 key 不是集合类型时，返回一个错误
	 * add one or more members to a set stored at a key
	 * 1) if the key doesn't exist, a new set by the key will be created,
	 *    and add the members to the set
	 * 2) if the key exists and not a set's key, then error happened
	 * @param key {const char*} 集合对象的键
	 *  the key of a set
	 * @param first_member {const char*} 第一个非 NULL 的成员
	 *  the first member of a variable args which isn't NULL, the last
	 *  arg of the args must be NULL indicating the end of args
	 * @return {int} 被添加到集合中的新元素的数量，不包括被忽略的元素
	 *  the number of elements that were added to the set, not including
	 *  all the elements already present into the set. -1 if error
	 *  happened or it isn't a set stored by the key.
	 */
	int sadd(const char* key, const char* first_member, ...);
	int sadd(const char* key, const std::vector<const char*>& memsbers);
	int sadd(const char* key, const std::vector<string>& members);
	int sadd(const char* key, const char* argv[], size_t argc);
	int sadd(const char* key, const char* argv[], const size_t lens[],
		size_t argc);

	/**
	 * 从集合对象中随机移除并返回某个成员
	 * remove and get one member from the set
	 * @param key {const char*} 集合对象的键
	 *  the key of the set
	 * @param buf {string&} 存储被移除的成员
	 *  store the member removed from the set
	 * @return {bool} 当 key 不存在或 key 是空集时返回 false
	 *  true if one member has been removed and got, false if the key
	 *  doesn't exist or it isn't a set stored at the key.
	 */
	bool spop(const char* key, string& buf);

	/**
	 * 获得集合对象中成员的数量
	 * get the number of members in a set stored at the key
	 * @param key {const char*} 集合对象的键
	 *  the key of the set
	 * @return {int} 返回该集合对象中成员数量，含义如下：
	 *  return int value as below:
	 *  -1：出错或非集合对象
	 *      error or it's not a set by the key
	 *   0：成员数量为空或该 key 不存在
	 *      the set is empty or the key doesn't exist
	 *  >0：成员数量非空
	 *      the number of members in the set
	 */
	int scard(const char* key);

	/**
	 * 返回集合 key 中的所有成员
	 * get all the members in a set stored at a key
	 * @param key {const char*} 集合对象的键值
	 *  the key of the set
	 * @param members {std::vector<string>*} 非空时存储结果集
	 *  if not NULL, it will store the members.
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
	 *  the number of elements got, -1 if error happened or it't not
	 *  a set by the key.
	 *
	 *  操作成功后可以通过以下任一方式获得数据
	 *  if successul, one of below ways can be used to get the result:
	 *  1、在调用方法中传入非空的存储结果对象的地址
	 *     the most easily way is to set a non-NULL result parameter
	 *     for this function 
	 *  2、基类方法 get_value 获得指定下标的元素数据
	 *     call redis_command::result_value with the specified subscript
	 *  3、基类方法 get_child 获得指定下标的元素对象(redis_result），然后再通过
	 *     redis_result::argv_to_string 方法获得元素数据
	 *     get redis_result object with the given subscript, and get the
	 *     element by redis_result::argv_to_string
	 *  4、基类方法 get_result 方法取得总结果集对象 redis_result，然后再通过
	 *     redis_result::get_child 获得一个元素对象，然后再通过方式 2 中指定
	 *     的方法获得该元素的数据
	 *     get redis_result object by redis_command::get_result, and get
	 *     the first element by redis_result::get_child, then get the
	 *     element by the way same as the way 2 above
	 *  5、基类方法 get_children 获得结果元素数组对象，再通过 redis_result 中
	 *     的方法 argv_to_string 从每一个元素对象中获得元素数据
	 *     get child array by redis_command::get_children, and get the
	 *     element from one of redis_result array by argv_to_string
	 */
	int smembers(const char* key, std::vector<string>* members);

	/**
	 * 将 member 元素从 src 集合移动到 dst 集合
	 * move a member from one set to another
	 * @param src {const char*} 源集合对象的键值
	 *  the source key of a set
	 * @param dst {const char*} 目标集合对象的键值
	 *  the destination key of a set
	 * @param member {const char*} 源集合对象的成员
	 *  the member in the source set
	 * @return {int} 返回值含义如下：
	 *  return int value as below:
	 *  -1：出错或源/目标对象有一个非集合对象
	 *      error happened, or one of source and destination isn't a set
	 *   0：源对象不存在或成员在源对象中不存在
	 *     the source set or the member doesn't exist
	 *   1：成功从源对象中将一个成员移动至目标对象中
	 *      move successfully the member from source set to
	 *      the destination set
	 */
	int smove(const char* src, const char* dst, const char* member);
	int smove(const char* src, const char* dst, const string& member);
	int smove(const char* src, const char* dst,
		const char* member, size_t len);

	/**
	 * 返回一个集合的全部成员，该集合是所有给定集合之间的差集
	 * return the members of the set resulting from the difference
	 * between the first set and all the successive sets.
	 * @param members {std::vector<string>*} 非空时存储结果集
	 *  if not NULL, it will store the members.
	 * @param first_key {const char*} 第一个非空的集合对象 key
	 *  the key of the first set in a variable sets list, the last one
	 *  must be NULL indicating the end of the sets list.
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
	 *  the number of elements got, -1 if error happened or it't not
	 *  a set by the key.
	 *  操作成功后可以通过以下任一方式获得数据
	 *  if successul, one of below ways can be used to get the result:
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
	 *     element from one of redis_result array by argv_to_string
	 */
	int sdiff(std::vector<string>* members, const char* first_key, ...);
	int sdiff(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sdiff(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * 返回一个集合的全部成员，该集合是所有给定集合的交集
	 * return the members of a set resulting from the intersection of
	 * all the give sets.
	 * @param members {std::vector<string>*} 非空时存储结果集
	 *  if not NULL, it will store the result
	 * @param first_key {const char*} 第一个集合对象 key（非NULL）
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  the last one must be NULL in the set list.
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
	 *  return the number of the members, -1 if error happened or
	 *  it't not a set by the key.
	 */
	int sinter(std::vector<string>* members, const char* first_key, ...);
	int sinter(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sinter(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * 返回一个集合的全部成员，该集合是所有给定集合的并集
	 * return the members of a set resulting from the union of all the
	 * given sets.
	 * @param members {std::vector<string>*} 非空时存储结果集
	 *  if not NULL, it will store the result
	 * @param first_key {const char*} 第一个集合对象 key（非NULL）
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sunion(std::vector<string>* members, const char* first_key, ...);
	int sunion(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sunion(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * 这个命令的作用和 SDIFF 类似，但它将结果保存到 dst 集合，而不是简单地返回结果集
	 * This command is equal to SDIFF, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} 目标集合对象键值
	 *  the key of the destination set
	 * @param first_key {const char*} 第一个非空的集合对象键值
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list. 
	 * @return {int} 结果集中的成员数量
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sdiffstore(const char* dst, const char* first_key, ...);
	int sdiffstore(const char* dst, const std::vector<const char*>& keys);
	int sdiffstore(const char* dst, const std::vector<string>& keys);

	/**
	 * 这个命令类似于 SINTER 命令，但它将结果保存到 dst 集合，而不是简单地返回结果集
	 * This command is equal to SINTER, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} 目标集合对象键值
	 *  the key of the destination set
	 * @param first_key {const char*} 第一个非空的集合对象键值
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} 结果集中的成员数量
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sinterstore(const char* dst, const char* first_key, ...);
	int sinterstore(const char* dst, const std::vector<const char*>& keys);
	int sinterstore(const char* dst, const std::vector<string>& keys);

	/**
	 * 这个命令类似于 SUNION 命令，但它将结果保存到 dst 集合，而不是简单地返回结果集
	 * This command is equal to SUNION, but instead of returning
	 * the resulting set, it is stored in destination.
	 * @param dst {const char*} 目标集合对象键值
	 *  the key of the destination set
	 * @param first_key {const char*} 第一个非空的集合对象键值
	 *  the key of the first set in a variable set list, which isn't NULL,
	 *  and the last arg must be NULL indicating the end of the set list.
	 * @return {int} 结果集中的成员数量
	 *  return the number of members, -1 if error happened or it's not
	 *  a set by the key.
	 */
	int sunionstore(const char* dst, const char* first_key, ...);
	int sunionstore(const char* dst, const std::vector<const char*>& keys);
	int sunionstore(const char* dst, const std::vector<string>& keys);

	/**
	 * 判断 member 元素是否集合 key 的成员
	 * determine if a given value is a member of a set
	 * @param key {const char*} 集合对象的键值
	 *  the key of a set
	 * @param member {const char*} 给定值
	 *  the given value
	 * @return {bool} 返回 true 表示是，否则可能是因为不是或出错或该 key 对象
	 *  非集合对象
	 *  true if the given is a member of the set, false if it's not a
	 *  member of the set, or error, or it's not a set by the key.
	 */
	bool sismember(const char* key, const char* member);
	bool sismember(const char* key, const char* member, size_t len);

	/**
	 * 如果命令执行时，只提供了 key 参数，那么返回集合中的一个随机元素，如果还同时指定
	 * 了元素个数，则会返回一个不超过该个数限制的结果集
	 * get one or multiple memebers from a set
	 * @param key {const char*} 集合对象的键值
	 *  the key of a set
	 * @param out 存储结果或结果集
	 *  store the result
	 * @return {int} 结果的个数，为 -1 表示出错，0 表示没有成员
	 *  the number of members, 0 if the set by the key is empty,
	 *  -1 if error happened.
	 */
	int srandmember(const char* key, string& out);
	int srandmember(const char* key, size_t n, std::vector<string>& out);

	/**
	 * 移除集合 key 中的一个或多个 member 元素，不存在的 member 元素会被忽略
	 * Remove the specified members from the set stored at key. if the
	 * member doesn't exist, it will be ignored.
	 * @param key {const char*} 集合对象的键值
	 *  the key of the set
	 * @param first_member {const char*} 需要被移除的成员列表的第一个非 NULL成员，
	 *  在变参的输入中需要将最后一个变参写 NULL
	 *  the first non-NULL member to be removed in a variable member list,
	 *  and the last one must be NULL indicating the end of the list.
	 * @retur {int} 被移除的成员元素的个数，当出错或非集合对象时返回 -1；当 key 不
	 *  存在或成员不存在时返回 0
	 *  the number of members be removed, 0 if the set is empty or the
	 *  key doesn't exist, -1 if error happened or it's not a set by key
	 */
	int srem(const char* key, const char* first_member, ...);
	int srem(const char* key, const std::vector<string>& members);
	int srem(const char* key, const std::vector<const char*>& members);
	int srem(const char* key, const char* members[],
		size_t lens[], size_t argc);

	/**
	 * 命令用于迭代当前数据库中的数据库键
	 * scan the members in a set stored at key
	 * @param key {const char*} 哈希键值
	 *  the key of a set
	 * @param cursor {int} 游标值，开始遍历时该值写 0
	 *  the cursor value, which is 0 at begin
	 * @param out {std::vector<string>&} 存储结果集，内部以追加方式将本次遍历
	 *  结果集合添加进该数组中，为防止因总结果集过大导致该数组溢出，用户可在调用本
	 *  函数前后清理该数组对象
	 *  store result in appending mode.
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
	int sscan(const char* key, int cursor, std::vector<string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl
