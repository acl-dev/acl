#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>

namespace acl
{

class ACL_CPP_API redis_slot
{
public:
	/**
	 * 构造函数
	 * constructor
	 * @param slot_min {size_t} 最小哈希槽值
	 *  the min hash-slot
	 * @param slot_max {size_t} 最大哈希槽值
	 *  the max hash-slot
	 * @param ip {const char*} 当前 redis-server 的 IP 地址
	 *  the given redis-server's ip
	 * @param port {int} 当前 redis-server 的监听端口
	 *  the listening port of the given redis-server
	 * 
	 */
	redis_slot(size_t slot_min, size_t slot_max,
		const char* ip, int port);
	redis_slot(const redis_slot& node);

	~redis_slot(void);

	/**
	 * 将一个 redis 哈希槽从结点添加至当前结点中
	 * add a slave slot node to the current node
	 * @param node {redis_slot*} 一个存储哈希槽的从结点
	 *  the slave slot node
	 */
	redis_slot& add_slave(redis_slot* node);

	/**
	 * 获得当前哈希槽结点的所有从结点
	 * get the slave nodes of the current node
	 * @return {const std::vector<redis_slot*>&}
	 */
	const std::vector<redis_slot*>& get_slaves() const
	{
		return slaves_;
	}

	/**
	 * 获得当前结点的 IP 地址
	 * get the ip of the current node
	 * @return {const char*}
	 */
	const char* get_ip(void) const
	{
		return ip_;
	}

	/**
	 * 获得当前结点的端口号
	 * get the port of the current node
	 * @return {int}
	 */
	int get_port(void) const
	{
		return port_;
	}

	/**
	 * 获得当前哈希槽结点的最小值
	 * get the min hash slot of the current node
	 * @return {size_t}
	 */
	size_t get_slot_min(void) const
	{
		return slot_min_;
	}

	/**
	 * 获得当前哈希槽结点的最大值
	 * get the max hash slot of the current node
	 * @return {size_t}
	 */
	size_t get_slot_max(void) const
	{
		return slot_max_;
	}

private:
	size_t slot_min_;
	size_t slot_max_;
	char ip_[128];
	int port_;

	std::vector<redis_slot*> slaves_;
};

} // namespace acl
