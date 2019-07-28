#pragma once
#include "../acl_cpp_define.hpp"
#include "master_base.hpp"
#include "../stdlib/thread_mutex.hpp"

#ifndef ACL_CLIENT_ONLY

struct ACL_VSTRING;

namespace acl {

class ACL_CPP_API master_udp : public master_base
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
	 * @return {bool} 监听是否成功
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 1);

protected:
	// 该类不能直接被实例化
	master_udp();
	virtual ~master_udp();

	/**
	 * 纯虚函数：当 UDP 流有数据可读时回调子类此函数，该方法在子线程中调用
	 * @param stream {socket_stream*}
	 */
	virtual void on_read(socket_stream* stream) = 0;

	/**
	 * 当绑定 UDP 地址成功后回调此虚方法，该方法在子线程中被调用
	 */
	virtual void proc_on_bind(socket_stream&) {}

	/**
	 * 当解绑 UDP 地址时回调此虚方法，该方法在子线程中被调用
	 */
	virtual void proc_on_unbind(socket_stream&) {}

	/**
	 * 当线程初始化时该虚方法将被调用
	 */
	virtual void thread_on_init(void) {}

	/**
	 * 获得本地监听的套接口流对象集合
	 * @return {const std::vector<socket_stream*>&}
	 */
	const std::vector<socket_stream*>& get_sstreams() const
	{
		return sstreams_;
	}

	/**
	 * 获得配置文件路径
	 * @return {const char*} 返回值为 NULL 表示没有设配置文件
	 */
	const char* get_conf_path(void) const;

public:
	void lock(void);
	void unlock(void);

private:
	std::vector<socket_stream*> sstreams_;
	thread_mutex lock_;

	void run(int argc, char** argv);
	void push_back(socket_stream* ss);
	void remove(socket_stream* ss);

private:
	// 当接收到一个客户端连接时回调此函数
	static void service_main(void*, ACL_VSTREAM*);

	// 当绑定地址成功后的回调函数
	static void service_on_bind(void*, ACL_VSTREAM*);

	// 当解绑地址时的回调函数
	static void service_on_unbind(void*, ACL_VSTREAM*);

	// 当进程切换用户身份后调用的回调函数
	static void service_pre_jail(void*);

	// 当进程切换用户身份后调用的回调函数
	static void service_init(void*);

	// 当进程退出时调用的回调函数
	static void service_exit(void*);

	// 当线程启动时调用的回调函数
	static void thread_init(void*);

	// 当进程收到 SIGHUP 信号后会回调本函数
	static int service_on_sighup(void*, ACL_VSTRING*);
};

} // namespace acl

#endif // ACL_CLIENT_ONLY
