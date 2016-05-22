#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_client_cluster;

class ACL_CPP_API redis_hyperloglog : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_hyperloglog(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_hyperloglog(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*, size_t)
	 */
	redis_hyperloglog(redis_client_cluster* cluster, size_t max_conns = 0);

	virtual ~redis_hyperloglog(void);

	/**
	 * 将任意数量的元素添加到指定的 HyperLogLog 里面
	 * add the specified elements to the specified HyperLogLog
	 * @param key {const char*} 指定 key 值
	 *  the key
	 * @param first_element {const char*} 元素集合的第一个元素值，非空字符串
	 *  the first element of the elements list, and the last must be NULL
	 *  in the elements list
	 * @return {int} 操作是否成功，同时表明是否发生了变更，返回值含义如下：
	 *  return the follow values:
	 *  1：操作成功，且数据发生了变更（新增数据或老数据发生变更）
	 *     successful, and the data was varied
	 *  0：修改老数据未发生变化
	 *     nothing was changed after modifying the old data
	 * -1：出错或对应的 key 对象非 hyperloglog 对象
	 *     error or the keh isn't a hyperloglog type
	 */
	int pfadd(const char* key, const char* first_element, ...);
	int pfadd(const char* key, const std::vector<const char*>& elements);
	int pfadd(const char* key, const std::vector<string>& elements);

	/**
	 * 获得给定键列表的 HyperLoglog 去重后元素的近似数量
	 * return the approximated cardinality of the set(s) observed by
	 * the hyperloglog at key(s)
	 * @param first_key {const char*} key 集合的第一个 key，非空字符串
	 *  the firs key which must not be NULL of the keys list, and the
	 *  last parameter must be NULL in keys' list
	 * @return {int} 键列表集合中经去重后元素的近似数量
	 */
	int pfcount(const char* first_key, ...);
	int pfcount(const std::vector<const char*>& keys);
	int pfcount(const std::vector<string>& keys);

	/**
	 * 将多个 HyperLogLog 合并（merge）为一个 HyperLogLog ， 合并后的
	 * HyperLogLog 的基数接近于所有输入 HyperLogLog 的可见集合的并集
	 * merge multiple different hyperloglogs into a single one
	 * @param dst {const char*} 目标存储 HyperLogLog 对象的键值
	 *  the single key as the destination
	 * @param first_src {const char*} 源对象集合中第一个源 HyperLogLog 对象的键
	 *  the first source key which must not be NULL in the sources list,
	 *  and the last one must be NULL showing the end of the list
	 * @return {bool} 操作是否成功，返回 false 表明出错或目标/源对象非
	 *  HyperLogLog 对象
	 *  true on success, false if the error or the dest/src are not
	 *  hyperloglog
	 */
	bool pfmerge(const char* dst, const char* first_src, ...);
	bool pfmerge(const char* dst, const std::vector<const char*>& keys);
	bool pfmerge(const char* dst, const std::vector<string>& keys);
};

} // namespace acl
