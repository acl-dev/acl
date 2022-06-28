#pragma once
#include "../stdlib/thread_mutex.hpp"
#include "../stream/aio_handle.hpp"
#include "../stream/aio_listen_stream.hpp"
#include "master_base.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTREAM;
struct ACL_VSTRING;

namespace acl {

class aio_handle;
class aio_socket_stream;

/**
 * acl_master 服务器框架中单线程非阻塞方式的模板类，该类对象只能有一个实例运行
 */
class ACL_CPP_API master_aio : public master_base, public aio_accept_callback
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
	 * @param ht {aio_handle_type} 事件引擎的类型
	 * @return {bool} 监听是否成功
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		aio_handle_type ht = ENGINE_SELECT);

	/**
	 * 获得异步IO的事件引擎句柄，通过此句柄，用户可以设置定时器等功能
	 * @return {aio_handle*}
	 */
	aio_handle* get_handle() const;

	/**
	 * 在 run_alone 模式下，通知服务器框架关闭引擎，退出程序
	 */
	void stop();

	/**
	 * 获得配置文件路径
	 * @return {const char*} 返回值为 NULL 表示没有设配置文件
	 */
	const char* get_conf_path(void) const;

protected:
	master_aio();
	virtual ~master_aio();

	/**
	 * 纯虚函数：当接收到一个客户端连接时调用此函数
	 * @param stream {aio_socket_stream*} 新接收到的客户端异步流对象
	 * @return {bool} 该函数如果返回 false 则通知服务器框架不再接收
	 *  远程客户端连接，否则继续接收客户端连接
	 */
	virtual bool on_accept(aio_socket_stream* stream) = 0;

private:
	aio_handle* handle_;
	/**
	 * 基类 aio_accept_callback 的虚函数实现
	 * @param client {aio_socket_stream*} 异步客户端流
	 * @return {bool} 返回 true 以通知监听流继续监听
	 */
	virtual bool accept_callback(aio_socket_stream* client);

private:
	thread_mutex lock_;
	void push_back(server_socket* ss);

private:
#if defined(_WIN32) || defined(_WIN64)
	// 当接收到一个客户端连接时回调此函数
	static void service_main(SOCKET, void*);
#else
	static void service_main(int, void*);
#endif

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
};

}  // namespace acl

#endif // ACL_CLIENT_ONLY
