#pragma once

class mymonitor : public acl::connect_monitor, public acl::aio_callback
{
public:
	mymonitor(acl::connect_manager& manager, const acl::string& proto);
	~mymonitor(void);

protected:
	/**
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

protected:
	// 重载父类 aio_callback 中的虚函数

	/**
	 * 客户端流的读成功回调过程
	 * @param data {char*} 读到的数据地址
	 * @param len {int} 读到的数据长度
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool read_callback(char* data, int len);

	/**
	 * 客户端流的超时回调过程
	 * @return {bool} 返回 true 表示继续，否则希望关闭该异步流
	 */
	bool timeout_callback();

	/**
	 * 客户端流的超时回调过程
	 */
	void close_callback();

private:
	acl::check_client* checker_;
	acl::string proto_;

	void sio_check_pop3(acl::check_client& checker, acl::socket_stream& conn);
	void sio_check_http(acl::check_client& checker, acl::socket_stream& conn);
};
