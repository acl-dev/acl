#include "stdafx.h"
#include "rpc_manager.h"

rpc_manager::rpc_manager(int max_threads /* = 10 */)
{
	// 因为本类实例是单例，会在程序 main 之前被调用，
	// 所以需要在此类中打开日志
	// 创建非阻塞框架句柄，并采用 WIN32 消息模式：acl::ENGINE_WINMSG
	handle_ = new acl::aio_handle(acl::ENGINE_WINMSG);
	// 创建 rpc 服务对象
	service_ = new acl::rpc_service(max_threads);
	// 打开消息服务
	if (service_->open(handle_) == false)
		logger_fatal("open service error: %s", acl::last_serror());
}

rpc_manager::~rpc_manager()
{
	delete service_;
	handle_->check();
	delete handle_;
	logger("rpc service destroy ok!");
}

void rpc_manager::fork(acl::rpc_request* req)
{
	service_->rpc_fork(req);
}
