#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <map>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/util.hpp"
#include "acl_cpp/ipc/rpc.hpp"
#include "acl_cpp/connpool/connect_manager.hpp"
#include "acl_cpp/connpool/connect_monitor.hpp"
#include "acl_cpp/connpool/check_client.hpp"
#endif
#include "check_timer.hpp"
#include "check_rpc.hpp"

namespace acl
{

connect_monitor::connect_monitor(connect_manager& manager)
: stop_(false)
, stop_graceful_(true)
, handle_(ENGINE_KERNEL)
, manager_(manager)
, check_inter_(1)
, conn_timeout_(10)
, rpc_service_(NULL)
{
}

connect_monitor::~connect_monitor(void)
{
}

connect_monitor& connect_monitor::open_rpc_service(int max_threads,
	const char* addr /* = NULL */)
{
	if (rpc_service_ != NULL) {
		return *this;
	}

	rpc_service_ = NEW rpc_service(max_threads);
	if (!rpc_service_->open(&handle_, addr)) {
		logger_fatal("open rpc_service error: %s", last_serror());
	}

	return *this;
}

connect_monitor& connect_monitor::set_check_inter(int n)
{
	check_inter_ = n;
	return *this;
}

connect_monitor& connect_monitor::set_conn_timeout(int n)
{
	conn_timeout_ = n;
	return *this;
}

void connect_monitor::stop(bool graceful)
{
	stop_ = true;
	stop_graceful_ = graceful;
}

void* connect_monitor::run(void)
{
	// 检查服务端连接状态定时器
	check_timer timer(*this, handle_, conn_timeout_);

	timer.keep_timer(true);  // 保持定时器
	handle_.set_timer(&timer, check_inter_ * 1000000);

	while (!stop_) {
		handle_.check();
	}

	// 等待定时器结束
	while (!timer.finish(stop_graceful_)) {
		handle_.check();
	}

	// 如果 rpc_service_ 对象非空则删除之
	delete rpc_service_;

	// 最后再检测一次，以尽量释放可能存在的异步对象
	handle_.check();
	return NULL;
}

void connect_monitor::on_open(check_client& checker)
{
	// 如果未设置 rpc 服务对象，则采用异步 IO 检测过程
	if (rpc_service_ == NULL) {
		checker.set_blocked(false);
		nio_check(checker, checker.get_conn());
	} else {
		// 设置检测对象为阻塞模式
		checker.set_blocked(true);

		// 创建 rpc 请求对象，将其放入线程池中运行，采用阻塞 IO 过程
		check_rpc* req = NEW check_rpc(*this, checker);
		rpc_service_->rpc_fork(req);
	}
}

void connect_monitor::nio_check(check_client& checker, aio_socket_stream&)
{
	// 设置状态表明该连接是存活的
	checker.set_alive(true);

	// 异步关闭连接检测对象
	checker.close();
}

void connect_monitor::sio_check(check_client& checker, socket_stream&)
{
	checker.set_alive(true);
}

} // namespace acl
