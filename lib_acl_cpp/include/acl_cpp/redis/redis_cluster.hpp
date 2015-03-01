#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"

namespace acl
{

class redis_pool;

class ACL_CPP_API redis_cluster : public connect_manager
{
public:
	/**
	 * 构造函数
	 * @param conn_timeout {int} 服务器连接超时时间(秒)
	 * @param rw_timeout {int}　网络 IO 读写超时时间(秒)
	 * @param max_slot {int} 哈希槽最大值
	 */
	redis_cluster(int conn_timeout, int rw_timeout, int max_slot = 16384);
	virtual ~redis_cluster();

	/**
	 * 根据哈希槽值获得对应的连接池
	 * @param slot {int} 哈希槽值
	 * @return {redis_pool*} 如果对应的哈希槽不存在则返回 NULL
	 */
	redis_pool* peek_slot(int slot);

	/**
	 * 设置哈希槽值对应的 redis 服务地址
	 * @param slot {int} 哈希槽值
	 * @param addr {const char*} redis 服务器地址
	 */
	void set_slot(int slot, const char* addr);

	/**
	 * 清除哈希槽对应的 redis 服务地址，以便于重新计算位置
	 * @param slot {int} 哈希槽值
	 */
	void clear_slot(int slot);

	/**
	 * 获得哈希槽最大值
	 * @return {int}
	 */
	int get_max_slot() const
	{
		return max_slot_;
	}

	//////////////////////////////////////////////////////////////////////
	/**
	 * 设置协议重定向次数的阀值，默认值为 15
	 * @param max {int} 重定向次数阀值，只有当该值 > 0 时才有效
	 */
	void set_redirect_max(int max);

	/**
	 * 设置协议重定向次数的阀值
	 * @return {int}
	 */
	int get_redirect_max() const
	{
		return redirect_max_;
	}

	/**
	 * 当重定向次数 >= 2 时允许休眠的时间(秒)，默认值为 1 秒，这样做的
	 * 好处是当一个 redis 服务主结点掉线后，其它从结点升级为主结点是需要
	 * 时间的(由 redis.conf 中的 cluster-node-timeout 配置项决定)，所以
	 * 为了在重定向的次数范围内不报错需要等待从结点升级为主结点
	 * @param n {int} 每次重定向时的休息时间(秒)，默认值为 1 秒
	 */
	void set_redirect_sleep(int n);

	/**
	 * 获得 set_redirect_sleep 设置的或默认的时间
	 * @return {int} 单位为秒
	 */
	int get_redirect_sleep() const
	{
		return redirect_sleep_;
	}

protected:
	/**
	 * 基类纯虚函数，用来创建连接池对象
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {int} 连接池的大小限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 */
	virtual acl::connect_pool* create_pool(const char* addr,
		int count, size_t idx);
private:
	int   conn_timeout_;
	int   rw_timeout_;
	int   max_slot_;
	const char**  slot_addrs_;
	std::vector<char*> addrs_;
	int   redirect_max_;
	int   redirect_sleep_;
};

} // namespace acl
