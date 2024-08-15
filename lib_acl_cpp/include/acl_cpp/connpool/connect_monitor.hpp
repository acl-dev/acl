#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/thread.hpp"
#include "../stream/aio_handle.hpp"
#include <vector>

namespace acl {

class aio_handle;
class check_client;
class connect_manager;
class rpc_service;
class socket_stream;
class aio_socket_stream;

class ACL_CPP_API connect_monitor : public thread {
public:
	/**
	 * 构造函数
	 * @param manager {connect_manager&}
	 * @param check_server {bool} 是否检查服务端服务可用性并将不用的服务地址置入黑名单中
	 */
	connect_monitor(connect_manager& manager, bool check_server = false);

	virtual ~connect_monitor();

	/**
	 * 当希望采用阻塞式检测服务端连接时，需要先调用本函数打开
	 * acl::rpc_service 阻塞接口处理服务；如果在初始化时不调用本函数，
	 * 则采用非阻塞方式进行 IO 检测
	 * @param max_threads {int} rpc_service 服务线程池中运行的最大线程数
	 * @param addr {const char*} 希望 rpc_service 服务监听的本机地址，可以
	 *  为本机的回地址或在 UNIX 平台下使用域套接口地址
	 * @return {connect_monitor&}
	 */
	connect_monitor& open_rpc_service(int max_threads,
		const char* addr = NULL);

	/**
	 * 设置检测定时器启动的时间间隔
	 * @param n {int} 时间间隔（秒）
	 * @return {connect_mointor&}
	 */
	connect_monitor& set_check_inter(int n);

	/**
	 * 设置连接被检测服务器的超时时间
	 * @param n {int} 超时时间（秒）
	 * @return {connect_monitor&}
	 */
	connect_monitor& set_conn_timeout(int n);

	/**
	 * 是否检查服务端服务可用性并将不用的服务地址置入黑名单中
	 * @return {bool}
	 */
	bool is_check_server() const {
		return check_server_;
	}

	/**
	 * 设置是否在连接检测器启动后自动关闭过期的空闲连接
	 * @param on {bool} 是否自动关闭过期的空闲连接
	 * @param kick_dead {bool} 是否检查所有连接的存活状态并关闭异常连接，当该参数
	 *  为 true 时，connect_client 的子类必须重载 alive() 虚方法，返回连接是否存活
	 * @param conns_min {size_t} > 0 表示尽量维持每个连接池中的最小活跃连接数
	 * @param step {bool} 每次检测连接池个数
	 * @return {connect_monitor&}
	 */
	connect_monitor& set_check_idle(bool on, bool kick_dead = false,
		size_t conns_min = 0, size_t step = 1);

	/**
	 * 是否自动检测并关闭过期空闲连接
	 * @return {bool}
	 */
	bool check_idle_on() const {
		return check_idle_on_;
	}

	/**
	 * 是否需要检查异常连接并关闭
	 * @return {bool}
	 */
	bool is_kick_dead() const {
		return kick_dead_;
	}

	/**
	 * 获得每个连接池希望保持的最小连接数
	 * @return {size_t}
	 */
	size_t get_conns_mininal() const {
		return conns_min_;
	}

	/**
	 * 当 check_idle_on() 返回 true 时，返回每次检测的连接数量限制
	 * @return {size_t}
	 */
	size_t get_check_idle_step() const {
		return check_idle_step_;
	}

	/**
	 * 停止检测线程
	 * @param graceful {bool} 是否文明地关闭检测过程，如果为 true
	 *  则会等所有的检测连接关闭后检测线程才返回；否则，则直接检测线程
	 *  直接返回，可能会造成一些正在检测的连接未被释放。正因如此，如果
	 *  连接池集群管理对象是进程内全局的，可以将此参数设为 false，如果
	 *  连接池集群管理对象在运行过程中需要被多次创建与释放，则应该设为 true
	 */
	void stop(bool graceful);

	/**
	 * 获得 connect_manager 引用对象
	 * @return {connect_manager&}
	 */
	connect_manager& get_manager() const {
		return manager_;
	}

	/**
	 * 虚函数，子类可以重载本函数用来进一步判断该连接是否是存活的，该回调
	 * 函数的运行空间为当前非阻塞检测线程的运行空间，因此在该回调函数中不
	 * 得有阻塞过程，否则将会阻塞整个非阻塞检测线程
	 * @param checker {check_client&} 服务端连接的检查对象，可以通过
	 *  check_client 类中的方法如下：
	 *  1) get_conn 获得非阻塞连接句柄
	 *  2) get_addr 获得服务端地址
	 *  3) set_alive 设置连接是否存活
	 *  4) close 关闭连接
	 */
	virtual void nio_check(check_client& checker, aio_socket_stream& conn);

	/**
	 * 同步 IO 检测虚函数，该函数在线程池的某个子线程空间中运行，子类可以
	 * 重载本函数以检测实际应用的网络连接存活状态，可以在本函数内有阻塞
	 * IO 过程
	 * @param checker {check_client&} 服务端连接的检查对象
	 *  check_client 类中允许调用的方法如下：
	 *  1) get_addr 获得服务端地址
	 *  2) set_alive 设置连接是否存活
	 *  check_client 类中禁止调用的方法如下：
	 *  1) get_conn 获得非阻塞连接句柄
	 *  2) close 关闭连接
	 */
	virtual void sio_check(check_client& checker, socket_stream& conn);

	/**
	 * 当连接成功时的回调方法，子类可以实现本方法
	 * @param cost {double} 从发起连接请求到超时的时间间隔（秒）
	 */
	virtual void on_connected(const check_client&, double cost) {
		(void) cost;
	}

	/**
	 * 当连接超时时的回调方法，子类可以实现本方法
	 * @param addr {const char*} 被检测的服务器地址，格式: ip:port
	 * @param cost {double} 从发起连接请求到超时的时间间隔（秒）
	 */
	virtual void on_timeout(const char* addr, double cost) {
		(void) addr;
		(void) cost;
	}

	/**
	 * 当连接服务器时被拒绝时的回调方法，子类可实现本方法
	 * @param addr {const char*} 被检测的服务器地址，格式: ip:port
	 * @param cost {double} 从发起连接请求到被断开的时间间隔（秒）
	 */
	virtual void on_refused(const char* addr, double cost) {
		(void) addr;
		(void) cost;
	}

public:
	// 虽然下面的函数是 public 的，但只供内部使用
	/**
	 * 当与服务端建立连接后调用此函数
	 * @param checker {check_client&}
	 */
	void on_open(check_client& checker);

protected:
	// 基类纯虚函数
	virtual void* run();

private:
	bool stop_;
	bool stop_graceful_;
	aio_handle handle_;			// 后台检测线程的非阻塞句柄
	connect_manager& manager_;		// 连接池集合管理对象
	bool check_server_;			// 是否检查服务端可用性
	int   check_inter_;			// 检测连接池状态的时间间隔(秒)
	int   conn_timeout_;			// 连接服务器的超时时间
	bool  check_idle_on_;			// 是否检测并关闭过期空闲连接
	bool  kick_dead_;			// 是否删除异常连接
	size_t conns_min_;			// 希望每个连接池的最小连接数
	size_t check_idle_step_;		// 每次检测连接数限制
	bool  check_dead_on_;			// 是否检测并关闭断开的连接
	size_t check_dead_step_;		// 每次检测异常连接的数量限制
	rpc_service* rpc_service_;		// 异步 RPC 通信服务句柄
};

} // namespace acl
