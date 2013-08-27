#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>

namespace acl
{

class connect_pool;

/**
 * connect pool 服务管理器，有获取连接池等功能
 */
class ACL_CPP_API connect_manager
{
public:
	connect_manager();
	virtual ~connect_manager();

	/**
	 * 初始化所有服务器的连接池，该函数内部调用 set 过程添加每个服务的连接池
	 * @param default_addr {const char*} 缺省的服务器地址，如果非空，
	 *  则在查询时优先使用此服务器
	 * @param addr_list {const char*} 所有服务器列表，可以为空
	 *  格式：IP:PORT:COUNT|IP:PORT:COUNT ...
	 *  如：127.0.0.1:7777:50|192.168.1.1:7777:10|127.0.0.1:7778
	 * @param default_count {int} 当 addr_list 中分隔的某个服务没有
	 *  COUNT 信息时便用此值
	 *  注：default_addr 和 addr_list 不能同时为空
	 */
	void init(const char* default_addr, const char* addr_list,
		int default_count);

	/**
	 * 添加服务器的客户端连接池
	 * @param addr {const char*} 服务器地址(ip:port)
	 * @param count {int} 连接池数量限制
	 * @return {connect_pool&} 返回新添加的连接池对象
	 */
	connect_pool& set(const char* addr, int count);

	/**
	 * 获得某个服务器的连接池
	 * @param addr {const char*} redis 服务器地址(ip:port)
	 * @return {connect_pool*} 返回空表示没有此服务
	 */
	connect_pool* get(const char* addr);

	/**
	 * 在读操作时，从连接池集群中获得一个连接池
	 * @return {connect_pool*} 返回一个连接池，返回 NULL 表示根据随机方式
	 *  无法获取有效连接池，应用需要重试其它方式
	 */
	connect_pool* peek();

	/**
	 * 获得所有的服务器的连接池，该连接池中包含缺省的服务连接池
	 * @return {std::vector<connect_pool*>&}
	 */
	std::vector<connect_pool*>& get_pools()
	{
		return pools_;
	}

	/**
	 * 获得缺省的服务器连接池
	 * @return {connect_pool*}
	 */
	connect_pool* get_default_pool()
	{
		return default_pool_;
	}

	/**
	 * 初始化定时器，该定时器会定期打印当前所有服务器集群的访问统计
	 * @param inter {int} 定时器的输出间隔
	 */
	void statistics_settimer(int inter = 1);

	/**
	 * 打印当前所有 redis 连接池的访问量
	 */
	void statistics();

protected:
	/**
	 * 纯虚函数，子类必须实现此函数用来创建连接池对象
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {int} 连接池的大小限制
	 */
	virtual connect_pool* create_pool(const char* addr, int count) = 0;
private:
	static void statistics_record(int, void* ctx);
	void statistics_timer();
private:
	string default_addr_;			// 缺省的服务地址
	connect_pool* default_pool_;		// 缺省的服务连接池
	std::vector<connect_pool*> pools_;	// 所有的服务连接池
	size_t service_size_;			// 缓存住当前总的服务器集群数量
	size_t service_idx_;			// 下一个要访问的的下标值
	locker lock_;				// 访问 pools_ 时的互斥锁
	int  stat_inter_;			// 统计访问量的定时器间隔

	// 设置除缺省服务之外的服务器集群
	void set_service_list(const char* addr_list, int count);
};

} // namespace acl
