#pragma once
#include "../acl_cpp_define.hpp"
#include <vector>
#include <map>
#include "../stdlib/string.hpp"
#include "../connpool/connect_manager.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

class sslbase_conf;

class redis_client_pool;

/**
 * redis 客户端集群类，通过将此类对象注册入 redis 客户端命令类(redis_command)，
 * 则使所有的客户端命令自动支持集群版 redis 命令。
 * redis client cluster class. The class's object is set in the redis_command
 * using redis_command::set_cluster(redis_cluster*), and all the redis client
 * command will support the redis cluster mode.
 */
class ACL_CPP_API redis_client_cluster : public connect_manager
{
public:
	/**
	 * 构造函数;
	 * constructor
	 * @param max_slot {int} 哈希槽最大值; the max hash-slot value of keys
	 */
	redis_client_cluster(int max_slot = 16384);
	virtual ~redis_client_cluster(void);

	/**
	 * 根据哈希槽值获得对应的连接池;
	 * get one connection pool with the given slot
	 * @param slot {int} 哈希槽值;
	 *  the hash-slot value of key
	 * @return {redis_client_pool*} 如果对应的哈希槽不存在则返回 NULL;
	 *  return the connection pool of the hash-slot, and return NULL
	 *  when the slot not exists
	 */
	redis_client_pool* peek_slot(int slot);

	/**
	 * 动态设置哈希槽值对应的 redis 服务地址，该函数被调用时内部有线程锁保护;
	 * dynamicly set redis-server addr with one slot, which is protected
	 * by thread mutex internal, no one will be set if the slot were
	 * beyyond the max hash-slot
	 * @param slot {int} 哈希槽值;
	 *  the hash-slot
	 * @param addr {const char*} redis 服务器地址;
	 *  one redis-server addr
	 */
	void set_slot(int slot, const char* addr);

	/**
	 * 给定 redis 集群中的一个结点，自动发现所有的哈希槽对应的结点地址
	 * according one node of the cluster, auto find all nodes with all
	 * slots range
	 * @param addr {const char*} 集群中的一个服务结点地址，格式 ip:port
	 *  on server node's addr of the cluster, addr format is "ip:port"
	 * @param max_conns {size_t} 集群中与每个结点所建连接池的最大连接限制
	 *  the max connections limit for each connection pool
	 * @param conn_timeout {int} 连接超时时间
	 *  set the connection timeout
	 * @param rw_timeout {int} IO 读写超时时间
	 *  set the network io timeout
	 */
	void set_all_slot(const char* addr, size_t max_conns,
		int conn_timeout = 30, int rw_timeout = 30);

	/**
	 * 动态清除哈希槽对应的 redis 服务地址，以便于重新计算位置，
	 * 内部有线程锁保护机制;
	 * dynamicly remove one slot and redis-server addr mapping, which is
	 * protected by thread mutex
	 * @param slot {int} 哈希槽值;
	 *  hash-slot value
	 */
	void clear_slot(int slot);

	/**
	 * 获得哈希槽最大值;
	 * get the max hash-slot
	 * @return {int}
	 */
	int get_max_slot() const
	{
		return max_slot_;
	}

	//////////////////////////////////////////////////////////////////////
	/**
	 * 设置协议重定向次数的阀值，默认值为 15;
	 * set redirect limit for MOVE/ASK, default is 15
	 * @param max {int} 重定向次数阀值，只有当该值 > 0 时才有效;
	 *  the redirect times limit for MOVE/ASK commands
	 */
	void set_redirect_max(int max);

	/**
	 * 设置协议重定向次数的阀值;
	 * get redirect limit of MOVE/ASK commands in one redis redirect process
	 * @return {int}
	 */
	int get_redirect_max() const
	{
		return redirect_max_;
	}

	/**
	 * 当重定向次数 >= 2 时允许休眠的时间(毫秒)，默认值为 100 毫秒，这样做的
	 * 好处是当一个 redis 服务主结点掉线后，其它从结点升级为主结点是需要
	 * 时间的(由 redis.conf 中的 cluster-node-timeout 配置项决定)，所以
	 * 为了在重定向的次数范围内不报错需要等待从结点升级为主结点;
	 * if redirect happenning more than 2 in one redis command process,
	 * the process can sleep for a one avoiding redirect too fast, you
	 * can set the waiting time with microsecond here, default value is
	 * 100 microseconds; this only happends when redis-server died.
	 * @param n {int} 每次重定向时的休息时间(毫秒)，默认值为 100 毫秒;
	 * microseonds to sleep when redirect times are more than 2,
	 * default is 100 ms
	 */
	void set_redirect_sleep(int n);

	/**
	 * 获得 set_redirect_sleep 设置的或默认的时间;
	 * get sleep time set by set_redirect_sleep function
	 * @return {int} 单位为毫秒;
	 *  return sleep value in microsecond
	 */
	int get_redirect_sleep() const
	{
		return redirect_sleep_;
	}

	/**
	 * 设置 SSL 通信方式下的配置句柄，内部缺省值为 NULL，如果设置了 SSL 连
	 * 接配置对象，则内部切换成 SSL 通信方式
	 * set SSL communication with Redis-server if ssl_conf not NULL
	 * @param ssl_conf {sslbase_conf*}
	 * @return {redis_client_cluster&}
	 */
	redis_client_cluster& set_ssl_conf(sslbase_conf* ssl_conf);

	/**
	 * 设置某个 redis 服务相应的连接密码
	 * set the password of one redis-server
	 * @param addr {const char*} 指定的某 redis 服务器地址，当该参数的值为
	 *  default 时，则指定了集群中所有 redis 服务器的缺省连接密码
	 *  the specified redis-server's addr, the default password of all
	 *  redis-server will be set when the addr's value is 'default'
	 * @param pass {const char*} 指定的 redis 服务器连接密码
	 *  the password of the specified redis-server
	 * @return {redis_client_cluster&}
	 */
	redis_client_cluster& set_password(const char* addr, const char* pass);

	/**
	 * 获得 redis 集群中服务节点与连接密码的对照表
	 * get all passwords of the redis cluster
	 * @return {const std::map<string, string>&}
	 */
	const std::map<string, string>& get_passwords(void) const
	{
		return passwds_;
	}

	/**
	 * 获得给定地址的 redis 节点的连接密码，返回 NULL 表示未设置
	 * get the connection password of the specified addr for one redis,
	 * NULL will be returned if password wasn't set
	 * @param addr {const char*}
	 * @return {const char*} return the specified node's connection password,
	 *  NULL returned if no password been set
	 */
	const char* get_password(const char* addr) const;

	/**
	 * 重定向至目标 redis 节点
	 * @param addr {const char*} 目标 redis 服务地址
	 * @param max_conns {size_t} 连接池最大连接数
	 * @return {redis_client*} 获得与目标 redis 节点的连接通信对象
	 */
	redis_client* redirect(const char* addr, size_t max_conns);

	/**
	 * 根据 redis 集群的槽号获得连接对象
	 * @param slot {int} redis 集群键值对应的存储槽槽号
	 * @return {redis_client*} 获得与目标 redis 节点的连接通信对象
	 */
	redis_client* peek_conn(int slot);

protected:
	/**
	 * 基类纯虚函数，用来创建连接池对象，该函数返回后由基类设置网络连接及IO 超时时间
	 * virtual function of base class, which is used to create
	 * the connection pool
	 * @param addr {const char*} 服务器监听地址，格式：ip:port;
	 * the server addr for the connection pool, such as ip:port
	 * @param count {size_t} 连接池的大小限制，该值没有 0 时则没有限制
	 * the max connections in one connection pool, if it's 0 there
	 * is no limit of the connections pool.
	 * @param idx {size_t} 该连接池对象在集合中的下标位置(从 0 开始);
	 * the index of the connection pool in pool array
	 */
	connect_pool* create_pool(const char* addr, size_t count, size_t idx);

private:
	int max_slot_;
	const char** slot_addrs_;
	std::vector<char*> addrs_;
	int redirect_max_;
	int redirect_sleep_;
	std::map<string, string> passwds_;
	sslbase_conf* ssl_conf_;

	redis_client* reopen(redis_command& cmd, redis_client* conn);
	redis_client* move(redis_command& cd, redis_client* conn, 
			const char* ptr, int nretried);
	redis_client* ask(redis_command& cd, redis_client* conn, 
			const char* ptr, int nretried);
	redis_client* cluster_down(redis_command& cd, redis_client* conn, 
			const char* ptr, int nretried);

public:
	const redis_result* run(redis_command& cmd, size_t nchild,
			int* timeout = NULL);
};

} // namespace acl

#endif // !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)
