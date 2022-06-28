#pragma once
#include "master_base.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTREAM;
struct ACL_EVENT;
struct ACL_VSTRING;

namespace acl {

class socket_stream;

/**
 * acl_master 服务器框架中进程方式的模板类，该类对象只能有一个实例运行
 */
class ACL_CPP_API master_proc : public master_base
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
	 * @param count {int} 当该值 > 0 时，则接收的连接次数达到此值且完成
	 *  后，该函数将返回，否则一直循环接收远程连接
	 * @return {bool} 监听是否成功
	 */
	bool run_alone(const char* addrs, const char* path = NULL, int count = 1);

	/**
	 * 获得配置文件路径
	 * @return {const char*} 返回值为 NULL 表示没有设配置文件
	 */
	const char* get_conf_path(void) const;

protected:
	master_proc();
	virtual ~master_proc();

	/**
	 * 纯虚函数：当接收到一个客户端连接时调用此函数
	 * @param stream {aio_socket_stream*} 新接收到的客户端异步流对象
	 * 注：该函数返回后，流连接将会被关闭，用户不应主动关闭该流
	 */
	virtual void on_accept(socket_stream* stream) = 0;

private:
	// 当接收到一个客户端连接时回调此函数
	static void service_main(void*, ACL_VSTREAM *stream);

	// 当监听一个服务地址时回调此函数
	static void service_on_listen(void*, ACL_VSTREAM*);

	// 当进程切换用户身份后调用的回调函数
	static void service_pre_jail(void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_init(void*);

	// 当进程退出时调用的回调函数
	static void service_exit(void*);

	// 当进程收到 SIGHUP 信号后会回调本函数
	static int service_on_sighup(void*, ACL_VSTRING*);

private:
	// 在单独运行方式下，该函数当监听套接字有新连接到达时被调用
	static void listen_callback(int event_type, ACL_EVENT*,
		ACL_VSTREAM*, void* context);

private:
	bool stop_;
	int  count_limit_;
	int  count_;
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
