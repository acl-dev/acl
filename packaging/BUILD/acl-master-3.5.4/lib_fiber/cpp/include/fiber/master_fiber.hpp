#pragma once
#include "fiber_cpp_define.hpp"
//#include "acl_cpp/master/master_base.hpp"

struct ACL_VSTREAM;

namespace acl {

class socket_stream;

/**
 * 基于协程方式的网络服务类
 */
class FIBER_CPP_API master_fiber : public master_base
{
public:
	/**
	 * 在 acl_master 框架下运行本网络服务对象
	 * @param argc {int} 传入的参数数组大小
	 * @param argv {char**} 传入的参数数组
	 */
	void run_daemon(int argc, char** argv);

	/**
	 * 以独立运行模式启动本网络服务对象
	 * @param addrs {const char*} 监听的本机服务地址列表，格式：
	 *  ip:port, ip:port, ...
	 * @param path {const char*} 非 NULL 指定配置文件路径
	 */
	bool run_alone(const char* addrs, const char* path = NULL);

	/**
	 * 获得配置文件路径
	 * @return {const char*} 返回值为 NULL 表示没有设配置文件
	 */
	const char* get_conf_path(void) const;

protected:
	master_fiber();

	virtual ~master_fiber();

	/**
	 * 虚函数，当协程服务器接收到客户端连接后调用本函数
	 * @param stream {socket_stream&} 客户端连接对象，本函数返回后，协程
	 *  服务框架将会关闭该连接对象
	 */
	virtual void on_accept(socket_stream& stream) = 0;

	/**
	 * 当线程初始化时该虚方法将被调用
	 */
	virtual void thread_on_init(void) {}

private:
	static void service_on_listen(void*, ACL_VSTREAM*);
	static void service_on_accept(void*, ACL_VSTREAM*);
	static void service_pre_jail(void*);
	static void service_init(void*);
	static void thread_init(void*); 
	static void service_exit(void*);
	static int  service_on_sighup(void*, ACL_VSTRING*);

	void run(int argc, char** argv);
};

} // namespace acl
