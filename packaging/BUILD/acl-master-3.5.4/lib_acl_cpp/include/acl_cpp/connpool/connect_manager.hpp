#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/string.hpp"
#include "../stdlib/locker.hpp"
#include "../stdlib/noncopyable.hpp"
#include <vector>
#include <map>

struct ACL_EVENT;

namespace acl
{

class connect_pool;
class connect_monitor;

// 内部使用数据结构
struct conns_pools {
	std::vector<connect_pool*> pools;
	size_t  check_next;			// 连接检测时的检测点下标
	size_t  conns_next;			// 下一个要访问的的下标值
	conns_pools(void)
	{
		check_next = 0;
		conns_next = 0;
	}
};

struct conn_config {
	string addr;
	size_t count;
	int    conn_timeout;
	int    rw_timeout;

	conn_config(void) {
		count        = 0;
		conn_timeout = 5;
		rw_timeout   = 5;
	}
};

/**
 * connect pool 服务管理器，有获取连接池等功能
 */
class ACL_CPP_API connect_manager : public noncopyable
{
public:
	connect_manager(void);
	virtual ~connect_manager(void);

	/**
	 * 是否将连接池与线程自动绑定，主要用于协程环境中，内部缺省值为 false，
	 * 该方法在本对象创建后仅能调用一次
	 * @param yes {bool}
	 */
	void bind_thread(bool yes);

	/**
	 * 初始化所有服务器的连接池，该函数调用 set 过程添加每个服务的连接池
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
	 * @param addr {const char*} 服务器地址，格式：ip:port
	 *  注意：调用本函数时每次仅能添加一个服务器地址，可以循环调用本方法
	 * @param count {size_t} 连接池数量限制, 如果该值设为 0，则不设置
	 *  连接池的连接上限
	 * @param conn_timeout {int} 网络连接时间(秒)
	 * @param rw_timeout {int} 网络 IO 超时时间(秒)
	 */
	void set(const char* addr, size_t count,
		int conn_timeout = 30, int rw_timeout = 30);

	/**
	 * 根据指定地址获取该地址对应的连接池配置对象
	 * @param addr {const char*} 目标连接池地址
	 * @param use_first {bool} 如果目标地址的配置对象不存在，是否允许使用
	 *  第一个地址配置对象
	 * @return {const conn_config*} 返回 NULL 表示不存在
	 */
	const conn_config* get_config(const char* addr, bool use_first = false);

	/**
	 * 设置连接池失败后重试的时间时间隔（秒），该函数可以在程序运行时被
	 * 调用，内部自动加锁
	 * @param n {int} 当该值 <= 0 时，若连接池出现问题则会立即被重试
	 */
	void set_retry_inter(int n);

	/**
	 * 设置连接池中空闲连接的空闲生存周期
	 * @param ttl {time_t} 空闲连接的生存周期，当该值 < 0 则表示空闲连接
	 *  不过期，== 0 时表示立刻过期，> 0 表示空闲该时间段后将被释放
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
	 * @param restore {bool} 当该服务结点被置为不可用时，该参数决定是否
	 *  自动将之恢复为可用状态
	 * @return {connect_pool*} 返回空表示没有此服务
	 */
	connect_pool* get(const char* addr, bool exclusive = true,
		bool restore = false);

	/**
	 * 从连接池集群中获得一个连接池，该函数采用轮循方式从连接池集合中获取
	 * 一个后端服务器的连接池，从而保证了完全的均匀性；该函数内部会自动对
	 * 连接池管理队列加锁
	 * 此外，该函数为虚接口，允许子类实现自己的轮循方式
	 * @return {connect_pool*} 返回一个连接池，返回指针永远非空
	 */
	virtual connect_pool* peek(void);

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
	void lock(void);

	/**
	 * 当用户重载了 peek 函数时，可以调用此函数对连接池管理过程加锁
	 */
	void unlock(void);

	/**
	 * 获得所有的服务器的连接池，该连接池中包含缺省的服务连接池
	 * @return {std::vector<connect_pool*>&}
	 */
	std::vector<connect_pool*>& get_pools(void);

	/**
	 * 检测连接池中的空闲连接，将过期的连接释放掉
	 * @param step {size_t} 每次检测连接池的个数
	 * @param left {size_t*} 非空时，将存储所有剩余连接个数总和
	 * @return {size_t} 被释放的空闲连接数
	 */
	size_t check_idle(size_t step, size_t* left = NULL);

	/**
	 * 获得连接池集合中连接池对象的个数
	 * @return {size_t}
	 */
	size_t size(void) const;

	/**
	 * 获得缺省的服务器连接池
	 * @return {connect_pool*} 当调用 init 函数的 default_addr 为空时
	 *  该函数返回 NULL
	 */
	connect_pool* get_default_pool(void)
	{
		return default_pool_;
	}

	/**
	 * 打印当前所有 redis 连接池的访问量
	 */
	void statistics(void);

	/**
	 * 启动后台非阻塞检测线程检测所有连接池连接状态
	 * @param monitor {connect_monitor*} 连接检测对象
	 * @return {bool} 是否正常启动了连接检测器，当返回 false 说明当前还有
	 *  正在运行的连接检测器，当想再次启动检测器时需要先调用 stop_monitor
	 */
	bool start_monitor(connect_monitor* monitor);

	/**
	 * 停止后台检测线程
	 * @param graceful {bool} 是否在关闭检测线程时需要等待所有的检测连接
	 *  关闭后才返回，当连接池集群对象为进程空间内不会多次分配与释放时，
	 *  则该值可以设为 false 从而使检测线程快速退出，否则应该等待所有检测
	 *  连接关闭后再使检测线程退出
	 * @return {connect_monitor*} 返回 start_monitor 设置的检测器，同时
	 *  内部的 monitor_ 成员自动置 NULL
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
	 * @param count {size_t} 连接池的大小限制，为 0 时，则连接池没有限制
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始)
	 * @return {connect_pool*} 返回创建的连接池对象
	 */
	virtual connect_pool* create_pool(const char* addr,
		size_t count, size_t idx) = 0;

protected:
	typedef std::vector<connect_pool*> pools_t;
	typedef pools_t::iterator          pools_it;
	typedef pools_t::const_iterator    pools_cit;

	typedef std::map<unsigned long, conns_pools*> manager_t;
	typedef manager_t::iterator                   manager_it;
	typedef manager_t::const_iterator             manager_cit;

	bool thread_binding_;			// 用于协程环境中与每个线程绑定
	string default_addr_;			// 缺省的服务地址
	connect_pool* default_pool_;		// 缺省的服务连接池

	std::map<string, conn_config> addrs_;	// 所有的服务端地址
	manager_t  manager_;

	locker lock_;				// 访问 pools_ 时的互斥锁
	int  stat_inter_;			// 统计访问量的定时器间隔
	int  retry_inter_;			// 连接池失败后重试的时间间隔
	time_t idle_ttl_;			// 空闲连接的生命周期
	int  check_inter_;			// 检查空闲连接的时间间隔
	connect_monitor* monitor_;		// 后台检测线程句柄

	// 设置除缺省服务之外的服务器集群
	void set_service_list(const char* addr_list, int count,
		int conn_timeout, int rw_timeout);
	conns_pools& get_pools_by_id(unsigned long id);
	connect_pool* create_pool(const conn_config& cf, size_t idx);
	void create_pools_for(pools_t& pools);

	void remove(pools_t& pools, const char* addr);
	void set_status(pools_t& pools, const char* addr, bool alive);

	unsigned long get_id(void) const;
	void get_key(const char* addr, string& key);
	void get_addr(const char* key, string& addr);
	connect_pool* add_pool(const char* addr);

	// 线程局部变量初始化时的回调方法
	static void thread_oninit(void);
	// 线程退出前需要回调此方法，用来释放内部创建的线程局部变量
	static void thread_onexit(void* ctx);
};

} // namespace acl
