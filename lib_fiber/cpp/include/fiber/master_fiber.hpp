#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/master/master_base.hpp"
#include "acl_cpp/stream/socket_stream.hpp"

namespace acl {

/**
 * 基于协程方式的网络服务类
 */
class master_fiber : public master_base
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
	 * @param count {unsigned int} 运行次数限制
	 */
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 0);

protected:
	master_fiber();
	virtual ~master_fiber();

	/**
	 * 纯虚函数，当协程服务器接收到客户端连接后调用本函数
	 * @param stream {socket_stream&} 客户端连接对象，本函数返回后，协程
	 *  服务框架将会关闭该连接对象
	 */
	virtual void on_accept(socket_stream& stream) = 0;

private:
	static void service_main(ACL_VSTREAM*, void*);
	static int  service_on_accept(ACL_VSTREAM*);
	static void service_pre_jail(void*);
	static void service_init(void*);
	static void service_exit(void*);
};

} // namespace acl
