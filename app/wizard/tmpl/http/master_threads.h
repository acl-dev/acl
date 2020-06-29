#pragma once

class http_service;

class master_service : public acl::master_threads
{
public:
	master_service(void);
	~master_service(void);

	http_service& get_service(void) const;

protected:
	/**
	 * @override
	 * 虚函数：当某个客户端连接有数据可读或关闭或出错时调用此函数
	 * @param stream {socket_stream*}
	 * @return {bool} 返回 false 则表示当函数返回后需要关闭连接，
	 *  否则表示需要保持长连接，如果该流出错，则应用应该返回 false
	 */
	bool thread_on_read(acl::socket_stream* stream);

	/**
	 * @override
	 * 当线程池中的某个线程获得一个连接时的回调函数，
	 * 子类可以做一些初始化工作
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 */
	bool thread_on_accept(acl::socket_stream* stream);

	/**
	 * @override
	 * 当某个网络连接的 IO 读写超时时的回调函数，如果该函数返回 true 则
	 * 表示继续等待下一次读写，否则则希望关闭该连接
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 */
	bool thread_on_timeout(acl::socket_stream* stream);

	/**
	 * @override
	 * 当与某个线程绑定的连接关闭时的回调函数
	 * @param stream {socket_stream*}
	 */
	void thread_on_close(acl::socket_stream* stream);

	/**
	 * @override
	 * 当线程池中一个新线程被创建时的回调函数
	 */
	void thread_on_init(void);

	/**
	 * @override
	 * 当线程池中一个线程退出时的回调函数
	 */
	void thread_on_exit(void);

	/**
	 * @override
	 * 在进程启动时，服务进程每成功监听一个本地地址，便调用本函数
	 * @param ss {acl::server_socket&} 监听对象
	 */
	void proc_on_listen(acl::server_socket& ss);

	/**
	 * @override
	 * 当进程切换用户身份后调用的回调函数，此函数被调用时，进程
	 * 的权限为普通受限级别
	 */
	void proc_on_init(void);

	/**
	 * @override
	 * 当子进程需要退出时框架将回调此函数，框架决定子进程是否退出取决于：
	 * 1) 如果此函数返回 true 则子进程立即退出，否则：
	 * 2) 如果该子进程所有客户端连接都已关闭，则子进程立即退出，否则：
	 * 3) 查看配置文件中的配置项(ioctl_quick_abort)，如果该配置项非 0 则
	 *    子进程立即退出，否则：
	 * 4) 等所有客户端连接关闭后才退出
	 * @param ncleints {size_t} 当前连接的客户端个数
	 * @param nthreads {size_t} 当前线程池中繁忙的工作线程个数
	 * @return {bool} 返回 false 表示当前子进程还不能退出，否则表示当前
	 *  子进程可以退出了
	 */
	bool proc_exit_timer(size_t nclients, size_t nthreads);

	/**
	 * @override
	 * 当进程退出前调用的回调函数
	 */
	void proc_on_exit(void);

	/**
	 * @override
	 * 当进程收到 SIGHUP 信号后的回调函数
	 */
	bool proc_on_sighup(acl::string&);

private:
	// redis 集群对象
	acl::redis_client_cluster* redis_;

	http_service* service_;
};
