#pragma once
#include "master_base.hpp"
#include "../stdlib/thread_mutex.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTREAM;
struct ACL_EVENT;
struct ACL_VSTRING;
struct acl_pthread_pool_t;

namespace acl {

class socket_stream;

/**
 * 线程池服务器框架类，该类为纯虚类，子类需要实现其中的纯虚函数，
 * 每一个进程仅能有一个该类对象实例，否则程序会被终止
 */
class ACL_CPP_API master_threads : public master_base
{
public:
	/**
	 * 开始运行，调用该函数是指该服务进程是在 acl_master 服务框架
	 * 控制之下运行，一般用于生产机状态
	 * @param argc {int} 从 main 中传递的第一个参数，表示参数个数
	 * @param argv {char**} 从 main 中传递的第二个参数
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * 在单独运行时的处理函数，用户可以调用此函数进行一些必要的调试工作
	 * @param addrs {const char*} 监听地址列表，格式：IP:PORT, IP:PORT...
	 * @param path {const char*} 配置文件全路径
	 * @param count {unsigned int} 循环服务的次数，达到此值后函数自动返回；
	 *  若该值为 0 则表示程序一直循环处理外来请求而不返回
	 * @param threads_count {int} 当该值大于 1 时表示自动采用线程池方式，
	 *  该值只有当 count != 1 时才有效，即若 count == 1 则仅运行一次就返回
	 *  且不会启动线程处理客户端请求
	 * @return {bool} 监听是否成功
	 *  注：count, threads_count 两个参数不再有效，将会使用配置文件中的
	 *  配置值 ioctl_use_limit(控制处理连接的个数) 及 ioctl_max_threads(
	 *  控制启动的最大线程数)
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 1, int threads_count = 1);
	
	/**
	 * 监听给定流的可读状态
	 * @param stream {socket_stream*}
	 */
	void thread_enable_read(socket_stream* stream);

	/**
	 * 不再监听给定流的可读状态
	 * @param stream {socket_stream*}
	 */
	void thread_disable_read(socket_stream* stream);

	/**
	 * 获得配置文件路径
	 * @return {const char*} 返回值为 NULL 表示没有设配置文件
	 */
	const char* get_conf_path(void) const;

	/**
	 * 获得当前线程池队列中积压的待处理任务数，该 API 可以方便应用决定何时
	 * 需要进行过载保护，在压力大的时候将后续的任务丢弃
	 * @return {size_t}
	 */
	size_t task_qlen(void) const;

public:
	/**
	 * 获得 lib_acl C 库中的线程池句柄
	 * @return {acl_pthread_pool_t*}
	 */
	acl_pthread_pool_t* threads_pool(void) const;

protected:
	// 该类不能直接被实例化
	master_threads();
	virtual ~master_threads();

	/**
	 * 纯虚函数：当某个客户端连接有数据可读或关闭或出错时调用此函数
	 * @param stream {socket_stream*}
	 * @return {bool} 返回 false 则表示当函数返回后需要关闭连接，
	 *  否则表示需要保持长连接，如果该流出错，则应用应该返回 false
	 */
	virtual bool thread_on_read(socket_stream* stream) = 0;

	/**
	 * 框架在调用 thread_on_read 后且其返回 true 后，会自动调用本函数
	 * 以判断是否监控流对象是否可读
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false，则框架不再监控该流对象
	 */
	virtual bool keep_read(socket_stream* stream)
	{
		(void) stream;
		return true;
	}

	/**
	 * 当线程池中的某个线程获得一个连接时的回调函数，子类可以做一些
	 * 初始化工作，该函数是在主线程的线程空间中运行
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 */
	virtual bool thread_on_accept(socket_stream* stream)
	{
		(void) stream;
		return true;
	}

	/**
	 * 当接收到一个客户端连接后，服务端回调此函数与客户端进行握手的操作，
	 * 该函数将在 thread_on_accept 之后被调用
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 */
	virtual bool thread_on_handshake(socket_stream *stream)
	{
		(void) stream;
		return true;
	}

	/**
	 * 当某个网络连接的 IO 读写超时时的回调函数，如果该函数返回 true 则
	 * 表示继续等待下一次读写，否则则希望关闭该连接
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，否则则要求
	 *  继续监听该连接
	 */
	virtual bool thread_on_timeout(socket_stream* stream)
	{
		(void) stream;
		return false;
	}

	/**
	 * 当与某个线程绑定的连接关闭时的回调函数
	 * @param stream {socket_stream*}
	 * 注：当在 thread_on_accept 返回 false 后流关闭时该函数并不会
	 * 被调用
	 */
	virtual void thread_on_close(socket_stream* stream) { (void) stream; }

	/**
	 * 当线程池中一个新线程被创建时的回调函数
	 */
	virtual void thread_on_init() {}

	/**
	 * 当线程池中一个线程退出时的回调函数
	 */
	virtual void thread_on_exit() {}

	/**
	 * 当子进程需要退出时框架将回调此函数，框架决定子进程是否退出取决于：
	 * 1) 如果此函数返回 true 则子进程立即退出，否则：
	 * 2) 如果该子进程所有客户端连接都已关闭，则子进程立即退出，否则：
	 * 3) 查看配置文件中的配置项(ioctl_quick_abort)，如果该配置项非 0 则
	 *    子进程立即退出，否则：
	 * 4) 等所有客户端连接关闭后才退出
	 * @param nclients {size_t} 当前连接的客户端个数
	 * @param nthreads {size_t} 当前线程池中繁忙的工作线程个数
	 * @return {bool} 返回 false 表示当前子进程还不能退出，否则表示当前
	 *  子进程可以退出了
	 */
	virtual bool proc_exit_timer(size_t nclients, size_t nthreads)
	{
		(void) nclients;
		(void) nthreads;
		return true;
	}

private:
	thread_mutex lock_;

	void push_back(server_socket* ss);
	void run(int argc, char** argv);

	// 当接收到一个客户端连接时回调此函数
	static int service_main(void*, ACL_VSTREAM*);

	// 当监听一个服务地址时回调此函数
	static void service_on_listen(void*, ACL_VSTREAM*);

	// 当接收到一个客户连接时的回调函数，可以进行一些初始化
	static int service_on_accept(void*, ACL_VSTREAM*);

	// 当接收到客户端连接后服务端需要与客户端做一些事先的握手动作时
	// 回调此函数，该函数会在 service_on_accept 之后被调用
	static int service_on_handshake(void*, ACL_VSTREAM*);

	// 当客户端连接读写超时时的回调函数
	static int service_on_timeout(void*, ACL_VSTREAM*);

	// 当客户端连接关闭时的回调函数
	static void service_on_close(void*, ACL_VSTREAM*);

	// 当进程切换用户身份后调用的回调函数
	static void service_pre_jail(void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_init(void*);

	// 当进程需要退出时调用此函数，由应用来确定进程是否需要退出
	static int service_exit_timer(void*, size_t, size_t);

	// 当进程退出时调用的回调函数
	static void service_exit(void*);

	// 当线程创建后调用的回调函数
	static int thread_init(void*);

	// 当线程退出前调用的回调函数
	static void thread_exit(void*);

	// 当进程收到 SIGHUP 信号后会回调本函数
	static int service_on_sighup(void*, ACL_VSTRING*);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
