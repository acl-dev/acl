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
	redis_set();

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_set(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_cluster*， size_t)
	 */
	redis_set(redis_cluster* cluster, size_t max_conns);

	virtual ~redis_set();

	/////////////////////////////////////////////////////////////////////

	/**
	 * 将一个或多个 member 元素加入到集合 key 当中，已经存在于集合的 member 元素
	 * 将被忽略;
	 * 1) 假如 key 不存在，则创建一个只包含 member 元素作成员的集合
	 * 2) 当 key 不是集合类型时，返回一个错误
	 * @param key {const char*} 集合对象的键
	 * @param first_member {const char*} 第一个非 NULL 的成员
	 * @return {int} 被添加到集合中的新元素的数量，不包括被忽略的元素
	 */
	int sadd(const char* key, const char* first_member, ...);
	int sadd(const char* key, const std::vector<const char*>& memsbers);
	int sadd(const char* key, const std::vector<string>& members);
	int sadd(const char* key, const char* argv[], size_t argc);
	int sadd(const char* key, const char* argv[], const size_t lens[],
		size_t argc);

	/**
	 * 从集合对象中随机移除并返回某个成员
	 * @param key {const char*} 集合对象的键
	 * @param buf {string&} 存储被移除的成员
	 * @return {bool} 当 key 不存在或 key 是空集时返回 false
	 */
	bool spop(const char* key, string& buf);

	/**
	 * 获得集合对象中成员的数量
	 * @param key {const char*} 集合对象的键
	 * @return {int} 返回该集合对象中成员数量，含义如下：
	 *  -1：出错或非集合对象
	 *   0：成员数量为空或该 key 不存在
	 *  >0：成员数量非空
	 */
	int scard(const char* key);

	/**
	 * 返回集合 key 中的所有成员
	 * @param key {const char*} 集合对象的键值
	 * @param members {std::vector<string>*} 非空时存储结果集
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
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
	int smembers(const char* key, std::vector<string>* members);

	/**
	 * 将 member 元素从 src 集合移动到 dst 集合
	 * @param src {const char*} 源集合对象的键值
	 * @param dst {const char*} 目标集合对象的键值
	 * @param member {const char*} 源集合对象的成员
	 * @return {int} 返回值含义如下：
	 *  -1：出错或源/目标对象有一个非集合对象
	 *   0：源对象不存在或成员在源对象中不存在
	 *   1：成功从源对象中将一个成员移动至目标对象中
	 */
	int smove(const char* src, const char* dst, const char* member);
	int smove(const char* src, const char* dst, const string& member);
	int smove(const char* src, const char* dst,
		const char* member, size_t len);

	/**
	 * 返回一个集合的全部成员，该集合是所有给定集合之间的差集
	 * @param members {std::vector<string>*} 非空时存储结果集
	 * @param first_key {const char*} 第一个非空的集合对象 key
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
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
	int sdiff(std::vector<string>* members, const char* first_key, ...);
	int sdiff(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sdiff(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * 返回一个集合的全部成员，该集合是所有给定集合的交集
	 * @param members {std::vector<string>*} 非空时存储结果集
	 * @param first_key {const char*} 第一个集合对象 key（非NULL）
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
	 */
	int sinter(std::vector<string>* members, const char* first_key, ...);
	int sinter(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sinter(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * 返回一个集合的全部成员，该集合是所有给定集合的并集
	 * @param members {std::vector<string>*} 非空时存储结果集
	 * @param first_key {const char*} 第一个集合对象 key（非NULL）
	 * @return {int} 结果集数量，返回 -1 表示出错或有一个 key 非集合对象
	 */
	int sunion(std::vector<string>* members, const char* first_key, ...);
	int sunion(const std::vector<const char*>& keys,
		std::vector<string>* members);
	int sunion(const std::vector<string>& keys,
		std::vector<string>* members);

	/**
	 * 这个命令的作用和 SDIFF 类似，但它将结果保存到 dst 集合，而不是简单地返回结果集
	 * @param dst {const char*} 目标集合对象键值
	 * @param first_key {const char*} 第一个非空的集合对象键值
	 * @return {int} 结果集中的成员数量
	 */
	int sdiffstore(const char* dst, const char* first_key, ...);
	int sdiffstore(const char* dst, const std::vector<const char*>& keys);
	int sdiffstore(const char* dst, const std::vector<string>& keys);

	/**
	 * 这个命令类似于 SINTER 命令，但它将结果保存到 dst 集合，而不是简单地返回结果集
	 * @param dst {const char*} 目标集合对象键值
	 * @param first_key {const char*} 第一个非空的集合对象键值
	 * @return {int} 结果集中的成员数量
	 */
	int sinterstore(const char* dst, const char* first_key, ...);
	int sinterstore(const char* dst, const std::vector<const char*>& keys);
	int sinterstore(const char* dst, const std::vector<string>& keys);

	/**
	 * 这个命令类似于 SUNION 命令，但它将结果保存到 dst 集合，而不是简单地返回结果集
	 * @param dst {const char*} 目标集合对象键值
	 * @param first_key {const char*} 第一个非空的集合对象键值
	 * @return {int} 结果集中的成员数量
	 */
	int sunionstore(const char* dst, const char* first_key, ...);
	int sunionstore(const char* dst, const std::vector<const char*>& keys);
	int sunionstore(const char* dst, const std::vector<string>& keys);

	/**
	 * 判断 member 元素是否集合 key 的成员
	 * @param key {const char*} 集合对象的键值
	 * @param member {const char*} 集合对象中的一个成员元素
	 * @return {bool} 返回 true 表示是，否则可能是因为不是或出错或该 key 对象
	 *  非集合对象
	 */
	bool sismember(const char* key, const char* member);
	bool sismember(const char* key, const char* member, size_t len);

	/**
	 * 如果命令执行时，只提供了 key 参数，那么返回集合中的一个随机元素，如果还同时指定
	 * 了元素个数，则会返回一个不超过该个数限制的结果集
	 * @param key {const char*} 集合对象的键值
	 * @param out 存储结果或结果集
	 * @return {int} 结果的个数，为 -1 表示出错，0 表示没有成员
	 */
	int srandmember(const char* key, string& out);
	int srandmember(const char* key, size_t n, std::vector<string>& out);

	/**
	 * 移除集合 key 中的一个或多个 member 元素，不存在的 member 元素会被忽略
	 * @param key {const char*} 集合对象的键值
	 * @param first_member {const char*} 需要被移除的成员列表的第一个非 NULL成员，
	 *  在变参的输入中需要将最后一个变参写 NULL
	 * @retur {int} 被移除的成员元素的个数，当出错或非集合对象时返回 -1；当 key 不
	 *  存在或成员不存在时返回 0
	 */
	int srem(const char* key, const char* first_member, ...);
	int srem(const char* key, const std::vector<string>& members);
	int srem(const char* key, const std::vector<const char*>& members);
	int srem(const char* key, const char* members[],
		size_t lens[], size_t argc);

	/**
	 * 命令用于迭代当前数据库中的数据库键
	 * @param key {const char*} 哈希键值
	 * @param cursor {int} 游标值，开始遍历时该值写 0
	 * @param out {std::vector<string>&} 存储结果集，内部以追加方式将本次遍历
	 *  结果集合添加进该数组中，为防止因总结果集过大导致该数组溢出，用户可在调用本
	 *  函数前后清理该数组对象
	 * @param pattern {const char*} 匹配模式，glob 风格，非空时有效
	 * @param count {const size_t*} 限定的结果集数量，非空指针时有效
	 * @return {int} 下一个游标位置，含义如下：
	 *   0：遍历结束
	 *  -1: 出错
	 *  >0: 游标的下一个位置
	 */
	int sscan(const char* key, int cursor, std::vector<string>& out,
		const char* pattern = NULL, const size_t* count = NULL);
};

} // namespace acl
