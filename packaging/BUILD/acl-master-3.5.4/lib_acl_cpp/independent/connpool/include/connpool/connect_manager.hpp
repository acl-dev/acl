#pragma once
#include <vector>

namespace acl_min
{

class connect_pool;
class locker;

/**
 * connect pool 服务管理器，有获取连接池等功能
 */
class connect_manager
{
public:
	connect_manager();
	virtual ~connect_manager();

	/**
	 * 初始化所有服务器的连接池，该函数内部调用 set 过程添加每个服务的连接池
	 * @param default_addr {const char*} 缺省的服务器地址，如果非空，
	 *  则在查询时优先使用此服务器
	 * @param addr_list {const char*} 所有服务器列表，可以为空
	 *  格式：IP:PORT:COUNT,IP:PORT:COUNT ...
	 *  如：127.0.0.1:7777:50, 192.168.1.1:7777:10, 127.0.0.1:7778
	 * @param default_count {int} 当 addr_list 中分隔的某个服务没有
	 *  COUNT 信息时便用此值
	 *  注：default_addr 和 addr_list 不能同时为空
	 * @return {bool} 如果返回 false 说明没有有效的地址
	 */
	bool init(const char* default_addr, const char* addr_list,
		int default_count);

	/**
	* 添加服务器的客户端连接池，，该函数可以在程序运行过程中
	* 被调用，因为内部会自动加锁
	 * @param addr {const char*} 服务器地址(ip:port)
	 * @param count {int} 连接池数量限制
	 * @return {connect_pool&} 返回新添加的连接池对象
	 */
	connect_pool& set(const char* addr, int count);

	/**
	 * 从连接池集群中删除某个地址的连接池，该函数可以在程序运行过程中
	 * 被调用，因为内部会自动加锁
	 * @param addr {const char*} 服务器地址(ip:port)
	 */
	void remove(const char* addr);

	/**
	 * 根据服务端地址获得该服务器的连接池
	 * @param addr {const char*} redis 服务器地址(ip:port)
	 * @param exclusive {bool} 是否需要互斥访问连接池数组，当需要动态
	 *  管理连接池集群时，该值应为 true
	 * @return {connect_pool*} 返回空表示没有此服务
	 */
	connect_pool* get(const char* addr, bool exclusive = true);

	/**
	 * 从连接池集群中获得一个连接池，该函数采用轮循方式从连接池集合中获取一个
	 * 后端服务器的连接池，从而保证了完全的均匀性；该函数内部会自动对连接池管理
	 * 队列加锁
	 * 此外，该函数为虚接口，允许子类实现自己的轮循方式
	 * @return {connect_pool*} 返回一个连接池，返回指针永远非空
	 */
	virtual connect_pool* peek();

	/**
	 * 从连接池集群中获得一个连接池，该函数采用哈希定位方式从集合中获取一个
	 * 后端服务器的连接池；子类可以重载此虚函数，采用自己的集群获取方式
	 * 该虚函数内部缺省采用 CRC32 的哈希算法；
	 * @param key {const char*} 键值字符串，如果该值为 NULL，则内部
	 *  自动切换到轮循方式
	 * @param exclusive {bool} 是否需要互斥访问连接池数组，当需要动态
	 *  管理连接池集群时，该值应为 true
	 * @return {connect_pool*} 返回一个可用的连接池，返回指针永远非空
	 */
	virtual connect_pool* peek(const char* key, bool exclusive = true);

	/**
	 * 当用户重载了 peek 函数时，可以调用此函数对连接池管理过程加锁
	 */
	void lock();

	/**
	 * 当用户重载了 peek 函数时，可以调用此函数对连接池管理过程加锁
	 */
	void unlock();

	/**
	 * 获得所有的服务器的连接池，该连接池中包含缺省的服务连接池
	 * @return {std::vector<connect_pool*>&}
	 */
	std::vector<connect_pool*>& get_pools()
	{
		return pools_;
	}

	/**
	 * 获得连接池集合中连接池对象的个数
	 * @return {size_t}
	 */
	size_t size() const
	{
		return pools_.size();
	}

	/**
	 * 获得缺省的服务器连接池
	 * @return {connect_pool*} 当调用 init 函数的 default_addr 为空时
	 *  该函数返回 NULL
	 */
	connect_pool* get_default_pool()
	{
		return default_pool_;
	}

protected:
	/**
	 * 纯虚函数，子类必须实现此函数用来创建连接池对象
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {int} 连接池的大小限制
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	virtual connect_pool* create_pool(const char* addr,
		int count, size_t idx) = 0;

	/**
	 * 打印当前所有 redis 连接池的访问量
	 */
	void statistics();

private:
	std::string default_addr_;		// 缺省的服务地址
	connect_pool* default_pool_;		// 缺省的服务连接池
	std::vector<connect_pool*> pools_;	// 所有的服务连接池
	size_t service_idx_;			// 下一个要访问的的下标值
	locker* lock_;				// 访问 pools_ 时的互斥锁
	int  stat_inter_;			// 统计访问量的定时器间隔

	// 设置除缺省服务之外的服务器集群
	void set_service_list(const char* addr_list, int count);
};

} // namespace acl_min
