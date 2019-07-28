#pragma once

class mymonitor : public acl::connect_monitor
{
public:
	mymonitor(acl::connect_manager& manager, const acl::string& proto);
	~mymonitor(void);

protected:
	/**
	 * @override
	 * 基类 connect_monitor 虚函数，重载本函数用来进一步判断该连接是否是存活的
	 * @param checker {check_client&} 服务端连接的检查对象，可以通过
	 *  check_client 类中的方法如下：
	 *  1) get_conn 获得非阻塞连接句柄
	 *  2) get_addr 获得服务端地址
	 *  3) set_alive 设置连接是否存活
	 *  4) close 关闭连接
	 */
	void nio_check(acl::check_client& checker,
		acl::aio_socket_stream& conn);

	/**
	 * @override
	 * 同步 IO 检测虚函数，该函数在线程池的某个子线程空间中运行，子类可以重载本函数
	 * 以检测实际应用的网络连接存活状态，可以在本函数内有阻塞 IO 过程
	 * @param checker {check_client&} 服务端连接的检查对象
	 *  check_client 类中允许调用的方法如下：
	 *  1) get_addr 获得服务端地址
	 *  2) set_alive 设置连接是否存活
	 *  check_client 类中禁止调用的方法如下：
	 *  1) get_conn 获得非阻塞连接句柄
	 *  2) close 关闭连接
	 */
	void sio_check(acl::check_client& checker, acl::socket_stream& conn);

	// @override
	void on_connected(const acl::check_client& checker, double cost);

	// @override
	void on_refuse(const acl::check_client& checker, double cost);

	// @override
	void on_timeout(const acl::check_client& checker, double cost);

private:
	acl::string proto_;
};
