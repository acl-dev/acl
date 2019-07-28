#pragma once
#include <list>

namespace acl_min
{

class connect_manager;
class connect_client;
class locker;

/**
 * 客户端连接池类，实现对连接池的动态管理，该类为纯虚类，需要子类实现
 * 纯虚函数 create_connect 用于创建与服务端的一个连接，当允许该类
 * 对象允许通过 set_delay_destroy() 设置延迟销毁时，该类的子类实例
 * 必须是动态对象
 */
class connect_pool
{
public:
	/**
	 * 构造函数
	 * @param addr {const char*} 服务器监听地址，格式：ip:port(domain:port)
	 * @param count {int} 连接池最大连接个数限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 */
	connect_pool(const char* addr, int count, size_t idx = 0);

	/**
	 * 该类当允许自行销毁时，类实例应为动态对象
	 */
	virtual ~connect_pool();

	/**
	 * 设置连接池异常的重试时间间隔
	 * @param retry_inter {int} 当连接断开后，重新再次打开连接的时间间隔(秒)，
	 *  当该值 <= 0 时表示允许连接断开后可以立即重连，否则必须超过该时间间隔后才
	 *  允许断开重连；未调用本函数时，内部缺省值为 1 秒
	 * @retrun {connect_pool&}
	 */
	connect_pool& set_retry_inter(int retry_inter);

	/**
	 * 设置连接池中空闲连接的空闲生存周期
	 * @param ttl {time_t} 空闲连接的生存周期，当该值 < 0 则表示空闲连接不过期，
	 *  == 0 时表示立刻过期，> 0 表示空闲该时间段后将被释放
	 * @return {connect_pool&}
	 */
	connect_pool& set_idle_ttl(time_t ttl);

	/**
	 * 从连接池中尝试性获取一个连接，当服务器不可用、距上次服务端连接异常时间间隔
	 * 未过期或连接池连接个数达到连接上限则将返回 NULL；当创建一个新的与服务器的
	 * 连接时失败，则该连接池会被置为不可用状态
	 * @return {connect_client*} 如果为空则表示该服务器连接池对象不可用
	 */
	connect_client* peek();

	/**
	 * 释放一个连接至连接池中，当该连接池对应的服务器不可用或调用者希望关闭该连接时，
	 * 则该连接将会被直接释放
	 * @param conn {redis_client*}
	 * @param keep {bool} 是否针对该连接保持长连接
	 */
	void put(connect_client* conn, bool keep = true);

	/**
	 * 检查连接池中空闲的连接，将过期的连接释放掉
	 * @param ttl {time_t} 空闲时间间隔超过此值的连接将被释放
	 * @param exclusive {bool} 内部是否需要加锁
	 * @return {int} 被释放的空闲连接个数
	 */
	int check_idle(time_t ttl, bool exclusive = true);

	/**
	 * 设置连接池的存活状态
	 * @param ok {bool} 设置该连接是否正常
	 */
	void set_alive(bool ok /* true | false */);

	/**
	 * 判断本连接池是否可用
	 * @return {bool}
	 */
	bool aliving(void) const
	{
		return alive_;
	}

	/**
	 * 获取连接池的服务器地址
	 * @return {const char*} 返回非空地址
	 */
	const char* get_addr() const
	{
		return addr_;
	}

	/**
	 * 获取连接池最大连接数限制
	 * @return {int}
	 */
	int get_count() const
	{
		return count_;
	}

	/**
	 * 获得该连接池对象在连接池集合中的下标位置
	 * @return {size_t}
	 */
	size_t get_idx() const
	{
		return idx_;
	}

	/**
	 * 重置统计计数器
	 * @param inter {int} 统计的时间间隔
	 */
	void reset_statistics(int inter);

	/**
	 * 获取该连接池总共被使用的次数
	 */
	unsigned long long get_total_used() const
	{
		return total_used_;
	}

	/**
	 * 获取该连接池当前的使用次数
	 * @return {unsigned long long}
	 */
	unsigned long long get_current_used() const
	{
		return current_used_;
	}

protected:
	virtual connect_client* create_connect() = 0;

	friend class connect_manager;

	/**
	 * 设置该连接池对象为延迟自销毁，当内部发现引用计数为 0 时会自行销毁
	 */
	void set_delay_destroy();

protected:
	bool  alive_;				// 是否属正常
	bool  delay_destroy_;			// 是否设置了延迟自销毁
	// 有问题的服务器的可以重试的时间间隔，不可用连接池对象再次被启用的时间间隔
	int   retry_inter_;
	time_t last_dead_;			// 该连接池对象上次不可用时的时间截

	size_t idx_;				// 该连接池对象在集合中的下标位置
	char  addr_[256];			// 连接池对应的服务器地址，IP:PORT
	int   max_;				// 最大连接数
	int   count_;				// 当前的连接数
	time_t idle_ttl_;			// 空闲连接的生命周期
	time_t last_check_;			// 上次检查空闲连接的时间截
	int   check_inter_;			// 检查空闲连接的时间间隔

	locker* lock_;				// 访问 pool_ 时的互斥锁
	unsigned long long total_used_;		// 该连接池的所有访问量
	unsigned long long current_used_;	// 某时间段内的访问量
	time_t last_;				// 上次记录的时间截
	std::list<connect_client*> pool_;	// 连接池集合
};

} // namespace acl_min
