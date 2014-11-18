#pragma once
#include <vector>
#include "acl_cpp/acl_cpp_define.hpp"
#include "acl_cpp/stdlib/thread.hpp"
#include "acl_cpp/stream/aio_handle.hpp"

namespace acl
{

class aio_handle;
class connect_manager;

class connect_monitor : public thread
{
public:
	connect_monitor(connect_manager& manager,
		int check_inter = 1, int conn_timeout = 10);
	~connect_monitor();

	/**
	 * 停止线程
	 * @param graceful {bool} 是否文明地关闭检测过程，如果为 true
	 *  则会等所有的检测连接关闭后检测线程才返回；否则，则直接检测线程
	 *  直接返回，可能会造成一些正在检测的连接未被释放。正因如此，如果
	 *  连接池集群管理对象是进程内全局的，可以将此参数设为 false，如果
	 *  连接池集群管理对象在运行过程中需要被多次创建与释放，则应该设为 true
	 */
	void stop(bool graceful);

protected:
	// 基类纯虚函数
	virtual void* run();

private:
	bool stop_;
	bool stop_graceful_;
	aio_handle handle_;			// 后台检测线程的非阻塞句柄
	connect_manager& manager_;		// 连接池集合管理对象
	int   check_inter_;			// 检测连接池状态的时间间隔(秒)
	int   conn_timeout_;			// 连接服务器的超时时间
};

} // namespace acl
