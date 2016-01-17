#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/locker.hpp"
#include <vector>

struct ACL_EVENT;

namespace acl
{

class connect_pool;
class connect_monitor;

/**
 * connect pool 服务管理器，有获取连接池等功能
 */
class ACL_CPP_API connect_manager
{
public:
	connect_manager(void);
	virtual ~connect_manager(void);

	/**
	 * 初始化所有服务器的连接池，该函数内部调用 set 过程添加每个服务的连接池
	 * @param default_addr {const char*} 缺省的服务器地址，如果非空，
	 *  则在查询时优先使用此服务器
	 * @param addr_list {const char*} 所有服务器列表，可以为空
	 *  格式: IP:PORT:COUNT;IP:PORT:COUNT;IP:PORT;IP:PORT ...
	 *    或  IP:PORT:COUNT,IP:PORT:COUNT,IP:PORT;IP:PORT ...
	 *  如：127.0.0.1:7777:50;192.168.1.1:7777:10;127.0.0.1:7778
	 * @param count {size_t} 当 addr_list 中分隔的某个服务没有
	 *  COUNT 信息时便用此值，当此值为 0 时，则不限制连接数上限
	 * @param conn_timeout {int} 网络连接时间(秒)
	 * @param rw_timeout {int} 网络 IO 超时时间(秒)
	 *  注：default_addr 和 addr_list 不能同时为空
	 */
	void init(const char* default_addr, const char* addr_list,
		size_t count, int conn_timeout = 30, int rw_timeout = 30);

	/**
	* 添加服务器的客户端连接池，该函数可以在程序运行时被调用，内部自动加锁
	 * @param addr {const char*} 服务器地址(ip:port)
	 * @param count {size_t} 连接池数量限制, 如果该值设为 0，则不设置
	 *  连接池的连接上限
	 * @param conn_timeout {int} 网络连接时间(秒)
	 * @param rw_timeout {int} 网络 IO 超时时间(秒)
	 * @return {connect_pool&} 返回新添加的连接池对象
	 */
	connect_pool& set(const char* addr, size_t count,
		int conn_timeout = 30, int rw_timeout = 30);

	/**
	 * 设置连接池失败后重试的时间时间隔（秒），该函数可以在程序运行时被调用，内部自动加锁
	 * @param n {int} 当该值 <= 0 时，若连接池出现问题则会立即被重试
	 */
	void set_retry_inter(int n);

	/**
	 * 设置连接池中空闲连接的空闲生存周期
	 * @param ttl {time_t} 空闲连接的生存周期，当该值 < 0 则表示空闲连接不过期，
	 *  == 0 时表示立刻过期，> 0 表示空闲该时间段后将被释放
	 */
	void set_idle_ttl(time_t ttl);

	/**
	 * 设置自动检查空闲连接的时间间隔，缺省值为 30 秒
	 * @param n {int} 时间间隔
	 */
	void set_check_inter(int n);

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
	 * @param restore {bool} 当该服务结点被置为不可用时，该参数决定是否自动
	 *  将之恢复为可用状态
	 * @return {connect_pool*} 返回空表示没有此服务
	 */
	connect_pool* get(const char* addr, bool exclusive = true,
		bool restore = false);

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

	/**
	 * 打印当前所有 redis 连接池的访问量
	 */
	void statistics();

	/**
	 * 启动后台非阻塞检测线程检测所有连接池连接状态
	 * @param monitor {connect_monitor*} 连接检测对象
	 * @return {bool} 是否正常启动了连接检测器，当返回 false 说明当前还有正在
	 *  运行的连接检测器，当想再次启动检测器时需要先调用 stop_monitor
	 */
	bool start_monitor(connect_monitor* monitor);

	/**
	 * 停止后台检测线程
	 * @param graceful {bool} 是否在关闭检测线程时需要等待所有的检测连接关闭后
	 *  才返回，当连接池集群对象为进程空间内不会多次分配与释放时，则该值可以设为 false
	 *  从而使检测线程快速退出，否则应该等待所有检测连接关闭后再使检测线程退出
	 * @return {connect_monitor*} 返回 start_monitor 设置的检测器，同时内部
	 *  的 monitor_ 成员自动置 NULL
	 */
	connect_monitor* stop_monitor(bool graceful = true);

	/**
	 * 设置某个连接池服务的存活状态，内部会自动加锁
	 * @param addr {const char*} 服务器地址，格式：ip:port
	 * @param alive {bool} 该服务器是否正常
	 */
	void set_pools_status(const char* addr, bool alive);

protected:
	/**
	 * 纯虚函数，子类必须实现此函数用来创建连接池对象
	 * @param addr {const char*} 服务器监听地址，格式：ip:port
	 * @param count {size_t} 连接池的大小限制，当该值为 0 时，则连接池没有限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	virtual connect_pool* create_pool(const char* addr,
		size_t count, size_t idx) = 0;

protected:
	string default_addr_;			// 缺省的服务地址
	connect_pool* default_pool_;		// 缺省的服务连接池
	std::vector<connect_pool*> pools_;	// 所有的服务连接池
	size_t service_idx_;			// 下一个要访问的的下标值
	locker lock_;				// 访问 pools_ 时的互斥锁
	int  stat_inter_;			// 统计访问量的定时器间隔
	int  retry_inter_;			// 连接池失败后重试的时间间隔
	time_t idle_ttl_;			// 空闲连接的生命周期
	int  check_inter_;			// 检查空闲连接的时间间隔
	connect_monitor* monitor_;		// 后台检测线程句柄

	// 设置除缺省服务之外的服务器集群
	void set_service_list(const char* addr_list, int count,
		int conn_timeout, int rw_timeout);
};

} // namespace acl
