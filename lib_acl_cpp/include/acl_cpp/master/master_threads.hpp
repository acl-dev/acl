#pragma once
#include "acl_cpp/master/master_base.hpp"
#include "acl_cpp/master/master_threads2.hpp"

struct ACL_VSTREAM;
struct ACL_EVENT;

namespace acl {

#if 1
typedef class master_threads2 master_threads;
#else

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
	 * @param addrs {const char*} 服务监听地址列表，格式：IP:PORT, IP:PORT...
	 * @param path {const char*} 配置文件全路径
	 * @param count {unsigned int} 循环服务的次数，达到此值后函数自动返回；
	 *  若该值为 0 则表示程序一直循环处理外来请求而不返回
	 * @param threads_count {int} 当该值大于 1 时表示自动采用线程池方式，
	 *  该值只有当 count != 1 时才有效，即若 count == 1 则仅运行一次就返回
	 *  且不会启动线程处理客户端请求
	 * @return {bool} 监听是否成功
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 1, int threads_count = 1);

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
	 * 当线程池中的某个线程获得一个连接时的回调函数，
	 * 子类可以做一些初始化工作
	 * @param stream {socket_stream*}
	 * @return {bool} 如果返回 false 则表示子类要求关闭连接，而不
	 *  必将该连接再传递至 thread_main 过程
	 *  注：当本函数返回 false 流关闭时并不调用 thread_on_close 过程
	 */
	virtual bool thread_on_accept(socket_stream* stream)
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

private:
	// 线程开始创建后的回调函数
	static int thread_begin(void* arg);

	// 线程结束运行前的回调函数
	static void thread_finish(void* arg);

	// 多线程情况下的处理函数
	static void thread_run(void* arg);

	// 仅运行一次
	static void run_once(ACL_VSTREAM* client);

	// 监听套被回调的函数
	static void listen_callback(int event_type, ACL_EVENT*,
		ACL_VSTREAM*, void *context);

	//////////////////////////////////////////////////////////////////

	// 当接收到一个客户端连接时回调此函数
	static int service_main(ACL_VSTREAM*, void*);

	// 当接收到一个客户连接时的回调函数，可以进行一些初始化
	static int service_on_accept(ACL_VSTREAM*);

	// 当客户端连接读写超时时的回调函数
	static int service_on_timeout(ACL_VSTREAM*, void*);

	// 当客户端连接关闭时的回调函数
	static void service_on_close(ACL_VSTREAM*, void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_pre_jail(void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_init(void*);

	// 当进程退出时调用的回调函数
	static void service_exit(void*);

	// 当线程创建后调用的回调函数
	static void thread_init(void*);

	// 当线程退出前调用的回调函数
	static void thread_exit(void*);
};

#endif

} // namespace acl
