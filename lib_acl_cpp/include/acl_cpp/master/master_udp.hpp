#pragma once
#include "acl_cpp/master/master_base.hpp"

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
	 * 纯虚函数：当 UDP 流有数据可读时回调子类此函数
	 * @param stream {socket_stream*}
	 */
	virtual void on_read(socket_stream* stream) = 0;

	/**
	 * 获得本地监听的套接口流对象集合
	 * @return {const std::vector<socket_stream*>&}
	 */
	const std::vector<socket_stream*>& get_sstreams() const
	{
		return sstreams_;
	}

private:
	std::vector<socket_stream*> sstreams_;

	void close_sstreams(void);

private:
	// 当接收到一个客户端连接时回调此函数
	static void service_main(ACL_VSTREAM *stream, char *service, char **argv);

	// 当进程切换用户身份后调用的回调函数
	static void service_pre_jail(char* service, char** argv);

	// 当进程切换用户身份后调用的回调函数
	static void service_init(char* service, char** argv);

	// 当进程退出时调用的回调函数
	static void service_exit(char* service, char** argv);

private:
	// 在单独运行方式下，该函数当监听套接字有新连接到达时被调用
	static void read_callback(int event_type, ACL_EVENT*,
		ACL_VSTREAM*, void* context);
};

} // namespace acl
