#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;

class ACL_CPP_API redis_hyperloglog : public redis_command
{
public:
	redis_hyperloglog(redis_client* conn = NULL);
	~redis_hyperloglog();

	/**
	 * 将任意数量的元素添加到指定的 HyperLogLog 里面
	 * @param key {const char*} 指定 key 值
	 * @param first_element {const char*} 元素集合的第一个元素值，非空字符串
	 * @return {int} 操作是否成功，同时表明是否发生了变更，返回值含义如下：
	 *  1：操作成功，且数据发生了变更（新增数据或老数据发生变更）
	 *  0：修改老数据未发生变化
	 * -1：出错或对应的 key 对象非 hyperloglog 对象
	 */
	int pfadd(const char* key, const char* first_element, ...);
	int pfadd(const char* key, const std::vector<const char*>& elements);
	int pfadd(const char* key, const std::vector<string>& elements);

	/**
	 * 获得给定键列表的 HyperLoglog 去重后元素的近似数量
	 * @param first_key {const char*} key 集合的第一个 key，非空字符串
	 * @return {int} 键列表集合中经去重后元素的近似数量
	 */
	int pfcount(const char* first_key, ...);
	int pfcount(const std::vector<const char*>& keys);
	int pfcount(const std::vector<string>& keys);

	/**
	 * 将多个 HyperLogLog 合并（merge）为一个 HyperLogLog ， 合并后的
	 * HyperLogLog 的基数接近于所有输入 HyperLogLog 的可见集合的并集
	 * @param dst {const char*} 目标存储 HyperLogLog 对象的键值
	 * @param first_src {const char*} 源对象集合中第一个源 HyperLogLog 对象的键
	 * @return {bool} 操作是否成功，返回 false 表明出错或目标/源对象非
	 *  HyperLogLog 对象
	 */
	bool pfmerge(const char* dst, const char* first_src, ...);
	bool pfmerge(const char* dst, const std::vector<const char*>& keys);
	bool pfmerge(const char* dst, const std::vector<string>& keys);
};

} // namespace acl
