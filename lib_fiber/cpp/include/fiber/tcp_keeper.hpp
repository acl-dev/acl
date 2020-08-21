#pragma once
#include "fiber_cpp_define.hpp"
#include <string>

namespace acl {

class keeper_waiter;
class socket_stream;
class thread_mutex;

/**
 * 独立线程用于预先与服务器创建空闲连接，客户端可以直接从该连接池中获取新连接，
 * 这对于 ping rtt 较长（如：10ms 以上）比较有价值，可以有效地减少因网络 rtt
 * 造成的连接时间损耗
 */
class tcp_keeper : public thread
{
public:
	tcp_keeper(void);
	~tcp_keeper(void);

	/**
	 * 设置建立网络连接的超时时间（秒）
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_timeout(int n);

	/**
	 * 设置网络套接字 IO 读写超时时间（秒）
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_rw_timeout(int n);

	/**
	 * 设置连接池中空闲连接的最小连接数
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_min(int n);

	/**
	 * 设置连接池中空闲连接的最大连接数
	 * @param n {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_max(int n);

	/**
	 * 设置网络连接的空闲时间（秒），空闲时间超过此值时连接将被关闭
	 * @param ttl {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_conn_ttl(int ttl);

	/**
	 * 设置每个连接池的空闲时间（秒），即当该连接池的空闲时间超过此值时
	 * 将被释放，从而便于系统回收内存资源
	 * @param ttl {int}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_pool_ttl(int ttl);

	/**
	 * 设置 rtt 阀值（秒），当网络连接时间超过此值时才会启用从连接池提取
	 * 连接方式，如果网络连接时间小于此值，则直接连接服务器
	 * @param rtt {double}
	 * @return {tcp_keeper&}
	 */
	tcp_keeper& set_rtt_min(double rtt);

	/**
	 * 从 tcp_keeper 对象中提取对应地址的网络连接对接
	 * @param addr {const char*} 服务器地址，格式：ip:port
	 * @param hit {bool*} 非空时，将存放该连接是否在连接池的空闲连接中命中
	 * @param sync {bool} 是否采用直连模式，如果采用直连模式，则内部不会
	 *  针对该地址预创连接池
	 * @return {socket_stream*} 返回 NULL 表示连接失败
	 */
	socket_stream* peek(const char* addr, bool* hit = NULL,
		bool sync = false);

	/**
	 * 停止 tcp_keeper 线程运行
	 */
	void stop(void);

protected:
	// @override
	void* run(void);

private:
	double rtt_min_;
	keeper_waiter* waiter_;
	std::map<std::string, double> addrs_;
	thread_mutex* lock_;

	bool direct(const char* addr, bool& found);
	void remove(const char* addr);
	void update(const char* addr, double cost);
};

} // namespace acl
